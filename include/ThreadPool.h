#pragma once
#include<vector>
#include<queue>
#include<thread>
#include<functional>
#include<mutex>
#include<condition_variable>

class ThreadPool {
public:
    ThreadPool(int poolSize) : abort(false) {
        for (int i = 0; i < poolSize; i++) {
            threadList.push_back(std::thread(&ThreadPool::runTask, this));
        }
    }

    void enqueueTask(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mut);
        taskQueue.push(task);
        lock.unlock();
        cv.notify_one();
    }

    void abortThreadPool() {
        std::unique_lock<std::mutex> lock(mut);
        abort = true;
        lock.unlock();
        cv.notify_all();
    }

    bool isAbort() {
        return (abort || taskQueue.empty());
    }

    ~ThreadPool() {
        std::unique_lock<std::mutex> lock(mut);
        abort = true;
        lock.unlock();
        cv.notify_all();

        for (std::thread& td : threadList) {
            td.join();
        }
    }

private:
    void runTask() {
        while (true) {
            std::unique_lock<std::mutex> lock(mut);
            cv.wait(lock, [this] { return abort || !taskQueue.empty(); });
            if (abort && taskQueue.empty()) return;
            auto task = std::move(taskQueue.front());
            taskQueue.pop();
            lock.unlock();
            task();
        }
    }

private:
    std::vector<std::thread> threadList;
    std::queue<std::function<void()>> taskQueue;
    std::mutex mut;
    std::condition_variable cv;
    bool abort;
};