#include <iostream>
#include <fstream>

#include <cstdlib>
#include <ctime>

#include "../src/smpl.h"

using namespace smpl;
using namespace std;

enum EventType {
    EventGenerate = 1, // события порождения новой заявки
    EventRelease, // событие освобождения устройства
    EventReserve, // событие резервирования устройства
    EventEnd // событие окончания моделирования
};

// Функция, запускающая моделирование
void model() {
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    // Для использования вывода в консоль: 
    // Engine * e = new Engine(&cout);

    // Вывод в файл
    ofstream fout("report.txt");
    Engine * e = new Engine(&fout);

    Device * dev = e->createDevice("Master");
    Queue * queue = e->createQueue("Accumulator");

    transact_t transactCounter = 1;

    e->schedule(EventGenerate, e->iRandom(14, 26), transactCounter);
    e->schedule(EventEnd, 480, 1e9);

    // Объявим переменную, в которой будем хранить номер текущего транзакта
    transact_t transact = 1;
    // И переменную, в которй будем хранить тип текущего события
    EventType event = EventEnd;
    do {
        // Снимаем событие с вершины
        pair<u64, transact_t> top = e->cause();
        // Разбираем пару на отдельные составляющие
        event = (EventType)top.first;
        transact = top.second;

        switch (event) {
            // Алгоритм обработки события порождения заявки
            case EventGenerate: {
                // Планируем сразу же событие резервирования устройства за этой заявкой
                e->schedule(EventReserve, 0, transact);
                // Увеличиваем счетчик транзактов
                transactCounter++;
                // Планируем поступление следующией заявки
                e->schedule(EventGenerate, e->iRandom(14, 26), transactCounter);
                break;
            }
            // Алгоритм обработки события резервирования устройства
            case EventReserve: {
                if (dev->status() == 0) {
                    // В случае, если устройство свободно, займем его заявкой
                    dev->reserve(transact);
                    // Запланируем освобождение устройства
                    e->schedule(EventRelease, e->iRandom(12, 20), transact);
                } else {
                    // Устройство занято, поместим заявку в очередь
                    queue->enqueue(transact, 0, 1);
                }
                break;
            }
            // Алгоритм обработки события освобождения устройства
            case EventRelease: {
                // Освобождаем устройство
                dev->release();
                if (queue->length() > 0) {
                    // В случае, если очередь не пуста, займем устройство заявкой с вершины очереди
                    smpl::u64 _tmp = 0;
                    transact = queue->head(_tmp);
                    e->schedule(EventReserve, 0, transact);
                }
                break;
            }
            // Конец моделирования
            case EventEnd: {
                break;
            }
            // Стандартный обработчик
            default: {
                cerr << "Unknown event type! " << event << endl;
                break;
            }
        }

    } while (event != EventEnd);

    // Построим отчет по текущему состоянию системы
    e->monitor();
    // Построим отчет по статистическим показателям системы
    e->report();

    delete e;
}

int main() {
    model();
    return 0;
}