// Ручная параллелизация разных задач на три потока: stream_1, stream_2 и main.
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <cstdlib>      // for rand() and srand()
#include <ctime>        // for time()
#include <chrono>
#include <thread>
#include <mutex>

const long long SIZE = 1000000LL;
std::mutex access_1, access_2, access_3;
template <typename T>
void Show_time(T const & t1_, T const & t2_ = std::chrono::high_resolution_clock::now());
void stream_1(const auto & t, std::vector<int> & v);          // функция
template <typename T>
class stream_2                                                // тип
{
private:
    const T & t;
    const std::vector<int> & v;     // источник входных данный
    const std::list<int> & l;       // источник входных данный
    std::vector<int> * result;      // запись выходных данный
public:
    stream_2(const T & tt, const std::vector<int> & vv, const std::list<int> & ll, std::vector<int> * v0) :
             t{tt}, v{vv}, l{ll}, result{v0} {};
    void operator()()               // функтор - размещение результата
    {
        std::scoped_lock lck {access_1, access_2};  // пока оба мьютекса не захватит, не будет продолжать поток
        std::cout << "Simultaneous time sorting(li) & (vi)\t";
        Show_time(t, std::chrono::high_resolution_clock::now());
        std::copy(l.begin(), l.end(), std::back_insert_iterator< std::vector<int> >(*result)); // vector<int>::push_back()
        std::copy(v.begin(), v.end(), std::back_insert_iterator< std::vector<int> >(*result));
        std::sort(result->begin(), result->end());
        std::cout << "stream_2 have ID " << std::this_thread::get_id() << ": is sort(li+vi) for\t";
        Show_time(t, std::chrono::high_resolution_clock::now());
    }   //не явно: access_1.unlock(); access_2.unlock();
};

int main()
{
    std::srand(std::time(0));                           // случайная инициализация rand()
    std::vector<int> vi0;
    std::vector<int> vi(SIZE); 
    std::generate(vi.begin(), vi.end(), std::rand);     // функтор без аргументов
    std::list<int> li(SIZE); 
    std::generate(li.begin(), li.end(), std::rand);
    
    auto t = std::chrono::high_resolution_clock::now(); // засечь время
    std::thread task_1( [&](){stream_1(t, vi);} );      // связать поток_1 (начать выполнять задачу_1), лямбда
    std::thread task_2{ stream_2{t, vi, li, &vi0} };    // связать поток_2 (начать выполнять задачу_2), функтор

    std::scoped_lock<std::mutex> lck {access_2};
    li.sort();
    std::cout << "stream_M have ID " << std::this_thread::get_id() << ": is sort(li)\tfor\t";
    Show_time(t);
    access_2.unlock();                          // надо явно освободить, т.к. его ожидает stream_2()
    if( task_1.joinable() ) task_1.join();      // соединить(замкнуть) main с поток_1 (будет ждать оставшиеся мсек.)
    if( task_2.joinable() ) task_2.join();      // соединить(замкнуть) main с поток_2 (будет ждать оставшиеся мсек.)
    std::cout << "Please press any key..";
    //std::cin.get();
	return 0;
}
template <typename T>
void Show_time(T const & t1_, T const & t2_)
{
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2_ - t1_).count() << "ms;\n";
}
void stream_1(const auto & t, std::vector<int> & v)     // функция
{
    std::scoped_lock<std::mutex> lck {access_1};
    std::sort(v.begin(), v.end());
    std::cout << "stream_1 have ID " << std::this_thread::get_id() << ": is sort(vi)\tfor\t";
    Show_time(t, std::chrono::high_resolution_clock::now());
}