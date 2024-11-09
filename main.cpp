#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iterator>
#include <thread>
#include <mutex>
#include <type_traits>

// Tomado de CPP Reference
void print (const auto rem, const auto& v) {
    std::cout << rem;
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << '\n';
}

void ejemplo_uso_merge_2_vectores() {

    // fill the vectors with random numbers
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(0, 9);

    std::vector<int> v1(10), v2(10);
    std::generate(v1.begin(), v1.end(), [&mt, &dis] { return dis(mt); });
    std::generate(v2.begin(), v2.end(), [&mt, &dis] { return dis(mt); });

    print("Originally:\nv1: ", v1);
    print("v2: ", v2);

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());

    print("Sorted:\nv1: ", v1);
    print("v2: ", v2);

    std::vector<int> dst(v1.size()+v2.size());
    std::merge(v1.begin(), v1.end(), v2.begin(), v2.end(), dst.begin());
    print("dst: ", dst);
}

void ejemplo_uso_merge_1_vector() {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(1, 50);

    std::vector<int> v1(30);
    std::generate(v1.begin(), v1.end(), [&mt, &dis] { return dis(mt); });
    print("v1: ", v1);
    std::sort(v1.begin(), std::next(v1.begin(), 15));
    std::sort(std::next(v1.begin(), 15), v1.end());
    print("v1: ", v1);
    std::inplace_merge(v1.begin(), std::next(v1.begin(), 15), v1.end());
    print("v1: ", v1);
}

template <typename Iterator>
void merge_sort(Iterator left, Iterator right) {
    auto d = std::distance(left, right);
    if (d <= 1) return;
    auto mid = std::next(left, std::distance(left, right) / 2);
    merge_sort(left, mid);
    merge_sort(mid, right);
    std::inplace_merge(left, mid, right);
}

template <typename Iterator>
void merge_sort_par(Iterator left, Iterator right, int depth=-1) {
    if (depth == -1) depth = std::log2(std::distance(left, right));
    if (std::distance(left, right) <= 1) return;
    auto mid = std::next(left, std::distance(left, right) / 2);
    if (depth > 0) {
        std::thread left_thread(merge_sort_par<Iterator>, left, mid, depth - 1);
        std::thread right_thread(merge_sort_par<Iterator>, mid, right, depth - 1);
        left_thread.join();
        right_thread.join();
    }
    else {
        merge_sort(left, mid);
        merge_sort(mid, right);
    }
    std::inplace_merge(left, mid, right);
}

void ejemplo_merge_sort() {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(1, 50);
    std::vector<int> v1(30);
    std::generate(v1.begin(), v1.end(), [&mt, &dis] { return dis(mt); });
    auto v2 = v1;
    auto v3 = v1;
    print("v1: ", v1);
    print("v2: ", v2);
    print("v3: ", v3);
    std::sort(v1.begin(), v1.end());
    print("v1: ", v1);
    print("v2: ", v2);
    print("v3: ", v3);
    merge_sort(v2.begin(), v2.end());
    print("v1: ", v1);
    print("v2: ", v2);
    print("v3: ", v3);
    merge_sort_par(v3.begin(), v3.end());
    print("v1: ", v1);
    print("v2: ", v2);
    print("v3: ", v3);
}

template <
        typename Iterator,
        typename T = typename std::iterator_traits<Iterator>::value_type>
Iterator binary_search_utec(Iterator left, Iterator right, T value, Iterator invalid_it = Iterator{}) {
    if (invalid_it == Iterator{}) invalid_it = right;
    if (std::distance(left, right) <= 0) return invalid_it;
    auto mid = std::next(left, std::distance(left, right) / 2);
    if (*mid == value) return mid;
    if (*mid > value)
        return binary_search_utec(left, std::prev(mid), value, invalid_it);
    else
        return binary_search_utec(std::next(mid), right, value, invalid_it);
}

std::mutex mtx;

template <
        typename Iterator,
        typename T = typename std::iterator_traits<Iterator>::value_type>
void binary_search_npar(Iterator left, Iterator right, T value, Iterator& result, Iterator invalid_it = Iterator{}) {
    if (std::distance(left, right) <= 0) return;
    auto mid = std::next(left, std::distance(left, right) / 2);

    if (result != invalid_it) {
        return;
    }
    if (*mid == value) {
        std::unique_lock ul(mtx);
        result = mid;
        return;
    }
    if (*mid > value)
        binary_search_npar(left, std::prev(mid), value, result, invalid_it);
    else
        binary_search_npar(std::next(mid), right, value, result, invalid_it);
}

template <
        typename Iterator,
        typename T = typename std::iterator_traits<Iterator>::value_type>
Iterator binary_search_par(Iterator left, Iterator right, T value) {

    // Parametros de los hilos
    auto nh = std::thread::hardware_concurrency();
    auto span = nh / std::distance(left, right);
    auto last_span = nh % std::distance(left, right);

    // iterador compartido
    auto result_it = right;

    // vector de hilos
    std::vector<std::thread> vh;

    // fork
    for (int i = 0; i < nh; ++i) {
        vh.emplace_back(binary_search_npar<Iterator>,
                        std::next(left, span*i), std::next(left, span*(i+1)),
                        value,
                        std::ref(result_it), right);
    }
    // join
    for (int i = 0; i < nh; ++i) {
        vh[i].join();
    }
    if (result_it == right && last_span > 0) {
        binary_search_npar(std::next(left, span*nh), right, value, result_it, right);
    }
    return result_it;
}


void ejemplo_busqueda_binaria() {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dis(1, 50);
    std::vector<int> v1(30);
    std::generate(v1.begin(), v1.end(), [&mt, &dis] { return dis(mt); });
    merge_sort_par(v1.begin(), v1.end());
    print("v1: ", v1);
    auto it = binary_search_utec(v1.begin(), v1.end(), 32);
    if (it != v1.end()) {
        std::cout << std::distance(v1.begin(), it) << std::endl;
        std::cout << *it << std::endl;
    }
    else {
        std::cout << "No found" << std::endl;
    }

    auto it2 = binary_search_par(v1.begin(), v1.end(), 32);
    if (it2 != v1.end()) {
        std::cout << std::distance(v1.begin(), it2) << std::endl;
        std::cout << *it2 << std::endl;
    }
    else {
        std::cout << "No found" << std::endl;
    }
}




int main() {
//    ejemplo_uso_merge_2_vectores();
//    ejemplo_uso_merge_1_vector();
//    ejemplo_merge_sort();
    ejemplo_busqueda_binaria();
    return 0;
}
