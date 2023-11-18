#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

struct MutexData
{
    std::mutex m_mutex;                                         // Мьютекс (синхронизация доступа к данным)
    std::condition_variable m_notification;                     // Ожидание уведомлений от других потоков
    int m_data;                                                 // Данные  
    bool m_isReady;                                             // Флаг (готовность к обработке)
};

/* Функция отправки данных и уведомления об их готовности */
void SendDataToChannel(MutexData& channel, int data)
{
    /* Блокируем мьютекс */
    std::unique_lock<std::mutex> block(channel.m_mutex);

    /* Записываем данные, готовые к обработке */
    channel.m_data = data;
    /* Устанавливаем флаг, указывающий, что данные готовы для обработки */
    channel.m_isReady = true;

    /* Отправляем сообщение одному из потоков, ожидающих данные для обработки */
    channel.m_notification.notify_one();

    /* Ждём обработки данных */
    channel.m_notification.wait(block, [&channel]
        {
            return !channel.m_isReady;
        });
}

/* Функция отправки данных в канал (поток-поставщик данных) */
void DataProviderThread(MutexData& channel)
{
    /* Отправка данных */
    for (int i = 1; i < 10; i++)
    {
        /* Приостанавливаем поток на 1 секунду перед отправкой новых данных */
        std::this_thread::sleep_for(std::chrono::seconds(1));
        /* Показываем, что задание отправляется */
        std::cout << "Поток-поставщик отправил задание №" << i << std::endl;
        /* Вызываем функцию для отправки данных */
        SendDataToChannel(channel, i);
    }
}

/* Функция обработки данных */
int ProcessDataFromChannel(MutexData& channel)
{
    /* Блокируем мьютекс */
    std::unique_lock<std::mutex> block(channel.m_mutex);

    /* Ждём обработки данных */
    channel.m_notification.wait(block, [&channel]
        {
            return channel.m_isReady;

        });

    /* Извлекаем данные для последующей обработки */
    int data = channel.m_data;
    /* Сбрасываем флаг, указывающий, что данные обработаны */
    channel.m_isReady = false;

    /* Отправляем сообщение одному из потоков, ожидающих данные для следующей передачи */
    channel.m_notification.notify_one();

    /* Возвращаем обработанные данные */
    return data;
}

/* Функция получения и обработки данных из канала (поток-потребитель данных) */
void DataProcessThread(MutexData& channel)
{
    /* Переменная для сохранения полученного из канала значения */
    int data;

    do
    {
        /* Вызываем функцию для извлечения данных из канала и их последующей обработки */
        data = ProcessDataFromChannel(channel);
        /* Показываем, что сообщение было получено */
        std::cout << "Поток-потребитель получил задание № " << data << std::endl;
    } while (data != 9);
}

int main()
{
    setlocale(LC_ALL, "Rus");

    /* Создаём экземпляр MutexData */
    MutexData mutexData{};

    /* Запускаем потоки поставщика */
    std::thread roviderThread(DataProviderThread, std::ref(mutexData));
    /* Запускаем потоки потребителя */
    std::thread processThread(DataProcessThread, std::ref(mutexData));

    /* Ждём завершения выполнения потоков */
    roviderThread.join();
    processThread.join();

    return 0;
}