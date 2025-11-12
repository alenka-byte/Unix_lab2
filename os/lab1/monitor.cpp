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
    void produce() {
        for (int i = 1; i <= 10; ++i) {
            this_thread::sleep_for(chrono::seconds(1));
            unique_lock<mutex> lock(mtx);

            if (ready == 1) {
                lock.unlock();
                continue; 
            }

            ready = 1;
            cout << "Поставщик: отправил событие с данными: " << i << endl;
            cv.notify_one();
            lock.unlock();
        }
    }

    void consume() {
        for (int i = 1; i <= 10; ++i) {
            unique_lock<mutex> lock(mtx);
            while (ready == 0) {
                cv.wait(lock);
                cout << "Потребитель: проснулся" << endl;
            }
            ready = 0;
            cout << "Потребитель: получил событие: " << i << endl;
            lock.unlock();
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
