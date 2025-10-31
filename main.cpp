#include "main.h"
#include "usb_cameras.h"
#include "utils.h"

using UsbCameraDevices = std::map<std::string, std::vector<Usb::FormatInfo>>;

static int Xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do
    {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

UsbCameraDevices GetCameraCapabilities()
{
    UsbCameraDevices devices;
    char dev_name[20]; // Буфер для имени устройства

    // 1. Перебираем все возможные устройства /dev/video*
    for (int i = 0; i < 64; ++i)
    {
        snprintf(dev_name, sizeof(dev_name), "/dev/video%d", i);

        // 2. Пытаемся открыть устройство
        // O_RDWR | O_NONBLOCK - стандарт для ioctl
        int fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd == -1)
        {
            continue; // Устройство не существует или нет прав
        }

        // 3. Проверяем возможности устройства
        v4l2_capability cap;
        std::memset(&cap, 0, sizeof(cap));
        if (Xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
        {
            close(fd);
            continue; // Не является V4L2 устройством
        }

        // Нас интересуют только устройства видеозахвата
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            close(fd);
            continue;
        }

        // --- Это веб-камера! Собираем информацию ---
        // Используем std::set для автоматического удаления дубликатов
        std::set<Usb::FormatInfo> format_set;

        // 4. Перечисляем форматы пикселей (YUYV, MJPEG и т.д.)
        v4l2_fmtdesc fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (Xioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0)
        {

            // 5. Для каждого формата перечисляем разрешения (frame sizes)
            v4l2_frmsizeenum frmsize;
            memset(&frmsize, 0, sizeof(frmsize));
            frmsize.pixel_format = fmt.pixelformat;

            while (Xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0)
            {

                // Нас интересуют только дискретные (точные) разрешения
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    int w = frmsize.discrete.width;
                    int h = frmsize.discrete.height;

                    // 6. Для каждого разрешения перечисляем FPS (frame intervals)
                    v4l2_frmivalenum frmival;
                    memset(&frmival, 0, sizeof(frmival));
                    frmival.pixel_format = fmt.pixelformat;
                    frmival.width = w;
                    frmival.height = h;

                    while (Xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
                    {

                        // Нас интересуют только дискретные интервалы
                        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                        {
                            if (frmival.discrete.numerator != 0)
                            {
                                // FPS = 1 / (интервал) = 1 / (числитель / знаменатель)
                                int fps = frmival.discrete.denominator / frmival.discrete.numerator;

                                format_set.insert({w, h, fps});
                            }
                        }
                        frmival.index++; // Следующий интервал
                    }
                }
                frmsize.index++; // Следующий размер
            }
            fmt.index++; // Следующий формат
        }
        // 7. Закрываем устройство и сохраняем результаты
        close(fd);

        if (!format_set.empty())
        {
            devices[dev_name] = std::vector<Usb::FormatInfo>(format_set.begin(), format_set.end());
            std::sort(devices[dev_name].begin(), devices[dev_name].end(), Utils::Comparator<Usb::FormatInfo>());
        }
    }
    return devices;
}

void RunThreadsUsb(UsbCameraDevices& cameras)
{
    if (cameras.empty())
    {
        std::cout << "Веб-камеры не найдены." << std::endl;
        return;
    }

    std::vector<std::thread> threads;
    size_t i = 0;
    for (const auto &pair : cameras)
    {
        threads.emplace_back(Usb::HandleUsbCamera, pair.first, pair.second[0]);
    }

    std::this_thread::sleep_for(std::chrono::microseconds(100));
    for (size_t i = 0; i < threads.size(); i++)
    {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}

int main(int argc, char *argv[])
{
    setenv("GST_V4L2_USE_LIBV4L2", "1", 1);
    gst_init(NULL, NULL);

    UsbCameraDevices cameras = GetCameraCapabilities();
    if (cameras.empty())
    {
        std::cout << "Веб-камеры не найдены." << std::endl;
        return -1;
    }

    std::cout << "Найдено " << cameras.size() << " веб-камер(ы):" << std::endl;
    for (const auto &pair : cameras)
    {
        std::cout << "\n--- Устройство: " << pair.first << " ---" << std::endl;

        for (const auto &format : pair.second)
        {
            std::cout << "  - " << format.width << "x" << format.height
                      << " @ " << format.fps << " FPS"
                      << std::endl;
        }
    }

    RunThreadsUsb(cameras);

    std::cout << "Main thread ID: " << std::this_thread::get_id() << "\n";
    return 0;
}
