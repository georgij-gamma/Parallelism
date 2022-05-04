// Параллелизация одного и того же контейнера на 2 и 4 потока
#include <iostream>
#include <vector>
#include <future>
#include <numeric>
#include <thread>
#include <chrono>
#include <cmath>

const long long SIZE = 10000000LL;
double accum(double* begin, double* end, double init);
double comp_two_streams(std::vector<double> & v);
double async_four_streams(std::vector<double> & v);

template <typename T>
void Show_time(T const & t1_, T const & t2_ = std::chrono::high_resolution_clock::now());

int main(int argc, char const *argv[])
{
    std::srand(std::time(0));                               // случайная инициализация rand()
    std::vector<double> vi(SIZE); 
    std::generate(vi.begin(), vi.end(), [](){ return std::pow(0.023, 0.019);} ); // заполняет контейнер значениями
    auto t0 = std::chrono::high_resolution_clock::now();    // засечь время
    auto sum_1 = comp_two_streams(vi);                      // 2х-поточный способ вычисления суммы элементов
    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield); 
    std::cout << "Sum of " << SIZE << " elements in two streams = " << sum_1 << ", for "
              << std::chrono::duration_cast<std::chrono::microseconds>
                (std::chrono::high_resolution_clock::now() - t0).count() << "us;\n";
    t0 = std::chrono::high_resolution_clock::now();         // засечь время
    auto sum_2 = async_four_streams(vi);                    // 4х-поточный способ вычисления суммы элементов
    std::cout << "Sum of " << SIZE << " elements in four streams = " << sum_2 << ", for "
              << std::chrono::duration_cast<std::chrono::microseconds>
                (std::chrono::high_resolution_clock::now() - t0).count() << "us;\n";
    std::cout << "Please press any key..";
    //std::cin.get();
    return 0;
}

double accum(double* begin, double* end, double init)       // Вычисление суммы элем.[beg:end) с начальным знач. init
{
    return std::accumulate(begin, end, init);
}

double comp_two_streams(std::vector<double> & v)
{
    using Task_type = double(double*, double*, double); // Тип задания
    std::packaged_task<Task_type> task_1 {accum};       // Упаковка задания accum(),
    std::packaged_task<Task_type> task_2 {accum};       // в качестве конструктора.
    std::future<double> fut1 {task_1.get_future()};     // Получение future task_1
    std::future<double> fut2 {task_2.get_future()};     // Получение future task_2
    double * first = &v[0];
// Запуск в поток алгоритма вычисления суммы элем. первой половины вектора (для task_1):
    std::thread stream_1{ std::move(task_1), first, first + v.size()/2, 0.0 };
// Запуск в поток алгоритма вычисления суммы элем. второй половины вектора (для task_2):
    std::thread stream_2{ std::move(task_2), first + v.size()/2, first + v.size(), 0.0 };
// packaged_task - дискриптор ресурса, владеет своим promise, поэтому применяется операция move()
    stream_1.join();
    stream_2.join();
    return fut1.get() + fut2.get();                     // Получение результата
}

double async_four_streams(std::vector<double> & v)
{
    if(v.size() < 10000)                                // Если контейнер мал,
        return accumulate(v.begin(), v.end(), 0.0);     // выполнить в один поток.

    auto v0 = &v[0];                                    // указатель на вектор
    auto size = v.size();                               // сохранить размер
    auto fut1 = std::async(accum, v0, v0 + size/4, 0.0);           // Первая четверть
    auto fut2 = std::async(accum, v0 + size/4, v0 + size/2, 0.0);  // Вторая четверть
    auto fut3 = std::async(accum, v0 + size/2, v0 + size*3/4, 0.0);// Третяя четверть
    auto fut4 = std::async(accum, v0 + size*3/4, v0 + size, 0.0);  // Четвёртая четверть
    return fut1.get() + fut2.get() + fut3.get() + fut4.get();      // Накопление и объединение результов
}