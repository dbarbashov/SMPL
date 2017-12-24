#ifndef SMPL_SMPL_H
#define SMPL_SMPL_H

#include <ostream>
#include <sstream>
#include <vector>
#include <set>
#include <list>

#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cassert>

namespace smpl
{
    typedef unsigned int uint;
    typedef unsigned long long u64;
    
    typedef u64 transact_t;

    class Event;
    class Device;
    class Queue;

    class Engine {
    private:
        /**
         * Выходной поток
         */
        std::ostream *outs;
        std::vector<Queue *> queues;
        std::vector<Device *> devices;
        std::set<Event> events;

        time_t _time;

        void justify(std::string &s, size_t sz);
        void justifyVec(std::vector<std::string> &vec, const std::vector<size_t> &widths);
        void fillVec(char c, std::vector<std::string> &vec, const std::vector<size_t> &widths);
        std::string surround(char c, const std::vector<std::string> &v);
        size_t getLen(std::string &s);
        std::string printTable(std::vector< std::vector<std::string> > table);
        template<typename T>
        std::string toString(T x);

    public:
        Engine(std::ostream *outputStream) : _time(0) {
            setlocale(LC_ALL, "ru_RU.UTF-8");
            outs = outputStream;
        }
        ~Engine() {
            reset();
        }
        void reset();
        /**
         * Определение устройства
         * @param name Название устройства
         * @return Созданное устройство
         */
        Device * createDevice(std::string name);
        /**
         * Определение очереди
         * @param name Название очереди
         * @return Созданная очередь
         */
        Queue * createQueue(std::string name);
        /**
         * Планирование события
         * Помещение в список нового события
         * @param eventId AE
         * @param time AT
         * @param transactId AJ
         */
        void schedule(u64 eventId, time_t time, transact_t transactId);
        /**
         * Обработка очередного события
         * @param eventId AE, ID события совершенного события
         * @param transactId AJ, ID транзакта
         */
        std::pair<u64, transact_t> cause();
        /**
         * Удаление события из списка
         * @param eventId AE
         * @param transactId AJ
         * @return Разность между текущим модельным временем и временем наступления удаленного события
         */
        time_t cancel(u64 eventId, transact_t transactId);
        time_t getTime();
        /**
         * Отражает на стандартном устройстве вывода или в файле состояние списка событий.
         * По каждому элементу списка выводится время свершения события, номер события и номер заявки.
         */
        void printEventsState();
        /**
         * Отображает список очередей и для каждого элемента выдается приоритет заявки,
         * время постановки заявки в очередь и номер заявки.
         */
        void printQueuesState();
        /**
         * Для каждого устройства выдается имя и номер занимающей его заявки
         */
        void printDevicesState();
        /**
         * Выводит текущее время и информацию по всем трем спискам
         */
        void monitor();
        void reportDevices();
        void reportQueues();
        void report();
        /**
         * Разыгрывает случайное число, равномерно распределенное на интервале от L до R
         * @param L
         * @param R
         * @return
         */
        uint iRandom(uint L, uint R);
        /**
         * Генерирует случайное число с плавающей точкой в отрезке [0; 1]
         * @return
         */
        double fRandom();
        uint negExp(uint x);
        uint poisson(uint x);
    };

    /**
     * Событие
     */
    class Event {
    public:
        /** T, время события */
        time_t time;
        /** E, номер события */
        u64 eventId;
        /** J, транзакт */
        transact_t transactId;

        friend bool operator<(const Event &a, const Event &b) {
            return a.time < b.time || (a.time == b.time && a.eventId < b.eventId);
        }
    };

    /**
     * Устройство
     */
    class Device {
    private:
        Engine * engine;

    public:
        /** Название устройства */
        std::string name;
        /** J, номер обрабатываемого транзакта. Если =0, то устройство свободно */
        transact_t currentTransactId;
        /** B, время последнего обращения */
        time_t lastTimeUsed;
        /** Z, счетчик запросов */
        size_t transactCount;
        /** SB, сумма периодов занятого состояния */
        time_t timeUsedSum;

        Device(const std::string &name, Engine *engine)
                : name(name), engine(engine), currentTransactId(0), lastTimeUsed(0),
                transactCount(0), timeUsedSum(0) {}
        /**
         * Резервирование устройства за транзактом
         * @param transactId AJ
         */
        void reserve(transact_t transactId);
        /**
         * Освобождение средства
         */
        void release();
        /**
         * Состояние средства 0 =свободно, иначе =номер транзакта
         * @return
         */
        transact_t status();
    };

    /**
     * Элемент очереди
     */
    class QueueItem {
    public:
        /** I, приоритет элемента очереди */
        u64 priority;
        /** J, идентификатор транзакта */
        transact_t transactId;
        /** T, время поступления элемента */
        time_t time;
        /** S, стадия обработки заявки */
        u64 stage;

        QueueItem(time_t time, transact_t transactId, u64 priority, u64 stage)
                : time(time), transactId(transactId), priority(priority), stage(stage) {}

        /**
         * Оператор сравнения двух элементов очереди.
         * Нужен для работы контейнеров стандартной библиотеки C++
         * @param a
         * @param b
         * @return
         */
        friend bool operator<(const QueueItem &a, const QueueItem &b) {
            return a.time < b.time || (a.time == b.time && a.priority < b.priority);
        }
    };

    /**
     * Очередь
     */
    class Queue {
    private:
        Engine * engine;
    public:
        /** Max, максимальная длина очереди */
        size_t maxLength;
        /** STQ, сумма произведений времени на длину очереди */
        u64 timeQueueSum;
        /** SW, сумма времен ожиданий */
        u64 waitTimeSum;
        /** SW2, сумма квадратов времен ожиданий */
        u64 waitTimeSumSquared;
        /** TLast, время последнего изменения длины очереди*/
        time_t lastTimeChanged;
        /** Count, счетчик элементов */
        size_t count;
        /** Контейнер */
        std::set<QueueItem> queue;
        /** Название очереди */
        std::string name;

        Queue(const std::string &name, Engine *engine)
                : name(name), engine(engine), maxLength(0), timeQueueSum(0),
                waitTimeSum(0), waitTimeSumSquared(0), lastTimeChanged(0), count(0) {
            assert(engine != NULL);
        }
        /**
         * Помещение транзакта в очередь
         * @param transactId AJ, транзакт
         * @param priority AI, приоритет
         * @param stage AS_, стадия обработки заявки
         */
        void enqueue(transact_t transactId, u64 priority, u64 stage);
        /**
         * Обработка транзакта
         * @param stage AS_, стадия обработки заявки
         * @return
         */
        transact_t head(u64 &stage);
        size_t length();
    };

    void Engine::justify(std::string &s, size_t sz) {
        size_t len = getLen(s);
        if (len >= sz)
            return;
        sz = sz - len;
        size_t l = sz/2;
        size_t r = sz - l;
        std::string leftPad;
        leftPad.append(l, ' ');
        s = leftPad + s;
        while (r --> 0) {
            s += " ";
        }
    }

    void Engine::justifyVec(std::vector<std::string> &vec, const std::vector<size_t> &widths) {
        assert(vec.size() == widths.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            justify(vec[i], widths[i]);
        }
    }

    void Engine::fillVec(char c, std::vector<std::string> &vec, const std::vector<size_t> &widths) {
        for (size_t i = 0; i < widths.size(); ++i) {
            std::string s(widths[i], c);
            vec.push_back(s);
        }
    }

    std::string Engine::surround(char c, const std::vector<std::string> &v) {
        std::string res;
        res.append(1, c);
        for (size_t i = 0; i < v.size(); i++) {
            res.append(v[i]);
            res.append(1, c);
        }
        return res;
    }

    /**
     * Возвращает настоящую длину UTF-8 строки
     * @param s
     * @return
     */
    size_t Engine::getLen(std::string &s) {
        size_t len = 0;
        std::string::iterator it = s.begin();
        while (it != s.end()) len += (*it++ & 0xc0) != 0x80;
        return len;
    }

    std::string Engine::printTable(std::vector<std::vector<std::string> > table) {
        assert(!table.empty());

        std::string res;
        const size_t MinColumnWidth = 5;

        std::vector<std::string> header = *table.begin();
        std::vector<size_t> colWidth(header.size(), MinColumnWidth);
        std::vector<std::string> sepList;
        std::string rowSeparator;

        for (size_t i = 0; i < table.size(); ++i) {
            for (size_t j = 0; j < table[i].size(); ++j) {
                colWidth[j] = std::max(colWidth[j], getLen(table[i][j]));
            }
        }

        fillVec('-', sepList, colWidth);
        rowSeparator = surround('+', sepList);

        justifyVec(header, colWidth);
        res += rowSeparator + "\n";
        res += surround('|', header) + "\n" + rowSeparator + "\n";

        for (size_t i = 1; i < table.size(); ++i) {
            justifyVec(table[i], colWidth);
            res += surround('|', table[i]) + "\n" + rowSeparator + "\n";
        }

        return res;
    }

    template<typename T>
    std::string Engine::toString(T x) {
        std::stringstream ss;
        ss << x;
        return ss.str();
    }

    void Engine::reset() {
        for (int i = 0; i < (int)queues.size(); ++i) {
            delete queues[i];
        }
        queues.empty();

        for (int i = 0; i < (int)devices.size(); ++i) {
            delete devices[i];
        }
        devices.empty();

        events.empty();
    }

    Device *Engine::createDevice(std::string name) {
        Device * d = new Device(name, this);
        devices.push_back(d);
        return d;
    }

    Queue *Engine::createQueue(std::string name) {
        Queue * q = new Queue(name, this);
        queues.push_back(q);
        return q;
    }

    void Engine::schedule(u64 eventId, time_t time, transact_t transactId) {
        assert(time >= 0);
        Event e;
        e.eventId = eventId;
        e.time = _time + time;
        e.transactId = transactId;
        events.insert(e);
    }

    std::pair<u64, transact_t> Engine::cause() {
        assert(events.size() > 0);

        Event e = *events.begin();
        events.erase(events.begin());
        _time = e.time;
        return std::make_pair(e.eventId, e.transactId);
    }

    time_t Engine::cancel(u64 eventId, transact_t transactId) {
        Event e;
        bool found = false;
        for (std::set<Event>::iterator it = events.begin(); it != events.end(); it++) {
            e = *it;
            if (e.transactId == transactId || e.eventId == eventId) {
                found = true;
                events.erase(it);
                break;
            }
        }
        assert(found);

        time_t res = e.time - _time;
        return res;
    }

    time_t Engine::getTime() {
        return _time;
    }

    void Engine::printEventsState() {
        std::vector< std::vector<std::string> > table(1);
        table[0].push_back("Время события");
        table[0].push_back("Номер события");
        table[0].push_back("Номер транзакта");
        table.resize(events.size() + 1);

        int i = 1;
        for (std::set<Event>::iterator it = events.begin(); it != events.end(); it++) {
            Event e = *it;
            table[i].push_back(toString(e.time));
            table[i].push_back(toString(e.eventId));
            table[i].push_back(toString(e.transactId));
            i++;
        }

        *outs << "Список событий:\n";
        *outs << printTable(table);
    }

    void Engine::printQueuesState() {
        std::vector< std::vector<std::string> > table(1);
        table[0].push_back("Приоритет");
        table[0].push_back("Время поступл.");
        table[0].push_back("Номер транзакта");

        for (size_t i = 0; i < queues.size(); ++i) {
            std::vector<std::string> rowQ(3, "");
            rowQ[0] = "Очередь:"; rowQ[1] = queues[i]->name;
            table.push_back(rowQ);

            std::set<QueueItem> container = queues[i]->queue;

            for (std::set<QueueItem>::iterator it = container.begin(); it != container.end(); it++) {
                QueueItem item = *it;
                std::vector<std::string> row(3);
                row[0] = toString(item.priority);
                row[1] = toString(item.time);
                row[2] = toString(item.transactId);
            }
        }

        *outs << "Список очередей:\n";
        *outs << printTable(table);
    }

    void Engine::printDevicesState() {
        std::vector< std::vector<std::string> > table(1);
        table[0].push_back("Имя устройства");
        table[0].push_back("Номер транзакта");
        table.resize(devices.size() + 1);

        for (size_t i = 0; i < devices.size(); ++i) {
            Device * dev = devices[i];
            table[i+1].push_back(toString(dev->name));
            table[i+1].push_back(toString(dev->currentTransactId));
        }

        *outs << "Список устройств:\n";
        *outs << printTable(table);
    }

    void Engine::monitor() {
        *outs << "*** Время моделирования: " << _time << "\n";
        printEventsState();
        printDevicesState();
        printQueuesState();
    }

    void Engine::reportDevices() {
        std::vector< std::vector<std::string> > table(1);

        table[0].push_back("Имя устройства");
        table[0].push_back("Ср.вр.зан.");
        table[0].push_back("% зан.вр.");
        table[0].push_back("Кол. запр.");
        table.resize(devices.size() + 1);

        for (size_t i = 0; i < devices.size(); ++i) {
            Device * dev = devices[i];
            table[i+1].push_back(toString(dev->name));
            table[i+1].push_back(dev->transactCount ?
                                 toString(dev->timeUsedSum * 1.0 / dev->transactCount) : "-");
            table[i+1].push_back(_time ?
                                 toString(dev->timeUsedSum * 1.0 / _time * 100) : "-");
            table[i+1].push_back(toString(dev->transactCount));
        }

        *outs << "Устройства\n";
        *outs << printTable(table);
    }

    void Engine::reportQueues() {
        std::vector< std::vector<std::string> > table(1);
        table[0].push_back("Имя очереди");
        table[0].push_back("Ср.вр.ожидания.");
        table[0].push_back("Ср.кв.откл.");
        table[0].push_back("Max");
        table[0].push_back("Ср.длина");
        table[0].push_back("Текущая длина");

        for (size_t i = 0; i < queues.size(); ++i) {
            std::vector<std::string> row(table[0].size(), "");
            Queue * q = queues[i];

            double avgWaitTime = q->count ? q->waitTimeSum / q->count : 0;

            row[0] = toString(q->name);
            row[1] = q->count ? toString(avgWaitTime) : " - ";
            row[2] = q->count ? toString(sqrt(q->waitTimeSumSquared/q->count - avgWaitTime*avgWaitTime)) : " - ";
            row[3] = toString(q->maxLength);
            row[4] = _time ? toString(q->timeQueueSum*1.0/_time) : " - ";
            row[5] = toString(q->length());
            table.push_back(row);
        }

        *outs << "Очереди:\n";
        *outs << printTable(table);
    }

    void Engine::report() {
        *outs << "Время моделирования: " << _time << " тактов\n";
        reportDevices();
        reportQueues();
    }

    uint Engine::iRandom(uint L, uint R) {
        if (L > R) {
            std::swap(L, R);
        }
        return L + rand()%(R-L);
    }

    double Engine::fRandom() {
        return rand() * 1.0 / RAND_MAX;
    }

    uint Engine::negExp(uint x) {
        return (uint) round(x * exp(-x * fRandom()));
    }

    uint Engine::poisson(uint x) {
        return (uint) round(x * -log(1.0 - fRandom()));
    }

    void Device::reserve(transact_t transactId) {
        assert(currentTransactId == 0);
        currentTransactId = transactId;
        lastTimeUsed = engine->getTime();
    }
    
    void Device::release() {
        assert(currentTransactId != 0);
        timeUsedSum += engine->getTime() - lastTimeUsed;
        transactCount++;
        currentTransactId = 0;
    }

    transact_t Device::status() {
        return currentTransactId;
    }

    void Queue::enqueue(transact_t transactId, u64 priority, u64 stage) {
        // TODO: check if transact already in queue
        queue.insert(QueueItem(engine->getTime(), transactId, priority, stage));

        timeQueueSum += (queue.size() - 1) * (engine->getTime() - lastTimeChanged);
        maxLength = std::max(maxLength, queue.size());
        lastTimeChanged = engine->getTime();
    }

    transact_t Queue::head(u64 &stage) {
        QueueItem qi = *queue.begin();
        queue.erase(queue.begin());

        timeQueueSum += (queue.size() + 1) * (engine->getTime() - lastTimeChanged);
        waitTimeSum += engine->getTime() - qi.time;
        waitTimeSumSquared += (engine->getTime() - qi.time) * (engine->getTime() - qi.time);
        lastTimeChanged = engine->getTime();
        count++;

        stage = qi.stage;
        return qi.transactId;
    }

    size_t Queue::length() {
        return queue.size();
    }
}

#endif //SMPL_SMPL_H
