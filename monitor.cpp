#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

class Monitor {
private:
    mutex mtx;
    condition_variable cv;
    int ready = 0;

public:
    // Метод поставщика
    void produce() {
        for (int i = 1; i <= 10; ++i) {
            // Задержка 1 секунда
            this_thread::sleep_for(chrono::seconds(1));

            lock_guard<mutex> lock(mtx);

            if (ready == 1) {
                continue; // Пропускаем если событие еще не обработано
            }

            ready = 1;
            cout << "Поставщик: отправил событие с данными: " << i << endl;

            // Будим потребителя
            cv.notify_one();
        }
    }

    // Метод потребителя
    void consume() {
        for (int i = 1; i <= 10; ++i) {
            unique_lock<mutex> lock(mtx);
            while (ready == 0) {
                cv.wait(lock);
                cout << "Потребитель: проснулся" << endl;
            }

            ready = 0;
            cout << "Потребитель: получил событие: " << i << endl;
        }
    }
};

int main() {
    system("chcp 1251");
    setlocale(LC_ALL, "Russian");
    Monitor monitor;
    cout << "================================" << endl;
    thread producer_thread(&Monitor::produce, &monitor);
    thread consumer_thread(&Monitor::consume, &monitor);
    producer_thread.join();
    consumer_thread.join();
    cout << "================================" << endl;
    cout << "Работа завершена." << endl;

    return 0;
}