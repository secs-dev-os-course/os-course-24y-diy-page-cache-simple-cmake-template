# Отчет по лабораторной работе №2
## Операционные системы

### Студент: Ляшенко Никита Андреевич
### Группа: P3309
### Преподаватель: Гиниятуллин Арслан
### Вариант: 
- Platform: Windows
- Политика вытеснения: LRU
- Доп: Реализация предзагрузки

## Задание
Для оптимизации работы с блочными устройствами в ОС существует кэш страниц с данными, которыми мы производим операции чтения и записи на диск. Такой кэш позволяет избежать высоких задержек при повторном доступе к данным, так как операция будет выполнена с данными в RAM, а не на диске (вспомним пирамиду памяти).

В данной лабораторной работе необходимо реализовать блочный кэш в пространстве пользователя в виде динамической библиотеки (dll или so). Политику вытеснения страниц и другие элементы задания необходимо получить у преподавателя.

При выполнении работы необходимо реализовать простой API для работы с файлами, предоставляющий пользователю следующие возможности:

1. Открытие файла по заданному пути файла, доступного для чтения. Процедура возвращает некоторый хэндл на файл. Пример: int lab2_open(const char *path).
2. Закрытие файла по хэндлу. Пример: `int lab2_close(int fd)`.
3. Чтение данных из файла. Пример:
`ssize_t lab2_read(int fd, void buf[.count], size_t count).`
4. Запись данных в файл. Пример:
`ssize_t lab2_write(int fd, const void buf[.count], size_t count).`
5. Перестановка позиции указателя на данные файла. Достаточно поддержать только абсолютные координаты. Пример:
​​​​​​`​off_t lab2_lseek(int fd, off_t offset, int whence).`
6. Синхронизация данных из кэша с диском. Пример:
`int lab2_fsync(int fd).`

Операции с диском разработанного блочного кеша должны производиться в обход page cache используемой ОС.

В рамках проверки работоспособности разработанного блочного кэша необходимо адаптировать указанную преподавателем программу-загрузчик из ЛР 1, добавив использование кэша. Запустите программу и убедитесь, что она корректно работает. Сравните производительность до и после.

### Ограничения
1. Программа (комплекс программ) должна быть реализован на языке C или C++.
2. Если по выданному варианту задана политика вытеснения Optimal, то необходимо предоставить пользователю возможность подсказать page cache, когда будет совершен следующий доступ к данным. Это можно сделать либо добавив параметр в процедуры read и write (например, ssize_t lab2_read(int fd, void buf[.count], size_t count, access_hint_t hint)), либо добавив еще одну функцию в API (например, int lab2_advice(int fd, off_t offset, access_hint_t hint)). access_hint_t в данном случае – это абсолютное время или временной интервал, по которому разработанное API будет определять время последующего доступа к данным.
3. Запрещено использовать высокоуровневые абстракции над системными вызовами. Необходимо использовать, в случае Unix, процедуры libc.

## Реализация

### Основные файлы
##### **Файлы кеша**
- cache.cpp
- cache.h
- cache_api.cpp
- cache_api.h
##### **Нагрузчики**
- io-thpt-read.cpp
- io-thpt-read-mod.cpp
- test-prefetching.cpp

**Измерение времени выполнения**
- Использование `std::chrono` для точного замера скорости доступа

#### Ключевые особенности
- Блочный кеш с объемом блока 4096 бит и 1024 блоками кеша
- Общий объем кеша 4 мб
- Создано api для удобного взаимодействия с кешом




#### Примеры использования
**Запуск стандартного нагрузчика**
```
PS D:\Itmo\5_SEM\os-course\diy-page-cache-Miqvet\build\bin> .\io_test_std.exe 5                                                                                    
Iteration 1 results:
  Read time: 0.0023436 seconds
  Data read: 5 MB
  Throughput: 2133.47 MB/s

Iteration 2 results:
  Read time: 0.0012324 seconds
  Data read: 5 MB
  Throughput: 4057.12 MB/s

Iteration 3 results:
  Read time: 0.0012231 seconds
  Data read: 5 MB
  Throughput: 4087.97 MB/s

Iteration 4 results:
  Read time: 0.0012285 seconds
  Data read: 5 MB
  Throughput: 4070 MB/s

Iteration 5 results:
  Read time: 0.0012896 seconds
  Data read: 5 MB
  Throughput: 3877.17 MB/s

Total results for process 6560:
Total read time: 0.0142778 seconds
Total data read: 25 MB
Average throughput: 1750.97 MB/s

```
**Запуск модифицированного нагрузчика**

```
PS D:\Itmo\5_SEM\os-course\diy-page-cache-Miqvet\build\bin> .\io_test_cache.exe 5
Iteration 1 results:
  Read time: 0.0622441 seconds
  Data read: 4 MB
  Throughput: 64.2631 MB/s

Iteration 2 results:
  Read time: 0.0006005 seconds
  Data read: 4 MB
  Throughput: 6661.12 MB/s

Iteration 3 results:
  Read time: 0.000552 seconds
  Data read: 4 MB
  Throughput: 7246.38 MB/s

Iteration 4 results:
  Read time: 0.0008279 seconds
  Data read: 4 MB
  Throughput: 4831.5 MB/s

Iteration 5 results:
  Read time: 0.0005664 seconds
  Data read: 4 MB
  Throughput: 7062.15 MB/s

Total results for process 16004:
Total read time: 0.0695271 seconds
Total data read: 20 MB
Average throughput: 287.658 MB/s
Middle data read (16384 bytes): fffffff2 ffffffae ffffffef 74 b ffffffcc 5f 13 ffffff98 79 ...
Middle read time: 0.0004644 seconds
Middle read speed: 33.6456 MB/s
```
### Анализ:
#### Производительность стандартного нагрузчика:

1. При использовании стандартного нагрузчика (io_test_std.exe) достигается высокая скорость чтения данных (средняя пропускная способность ~4 ГБ/с).
Это связано с использованием системного кеша страниц ОС, который оптимизирован для работы с файлами и эффективно использует RAM.
Производительность модифицированного нагрузчика:

2. Модифицированный нагрузчик (io_test_cache.exe) в первой итерации демонстрирует низкую пропускную способность (~64 МБ/с). Это объясняется тем, что данные загружаются непосредственно с диска, минуя системный кеш.
При повторных итерациях скорость увеличивается до ~7 ГБ/с благодаря тому, что данные остаются в пользовательском кеше в RAM, обеспечивая быстрый доступ без обращения к диску.

### Дополнительное задание
#### Вариант: Реализовать предзагрузку блоков в кеш


#### Релализация 

#### Вывод
```
Generating test file...
File generated: test_file.dat, size: 65536 bytes.
Testing cache prefetching...
Reading blocks sequentially...
Read block 0, bytes: 4096, speed: 2296.34 KB/s
Read block 1, bytes: 4096, speed: 4137.36 KB/s
Read block 2, bytes: 4096, speed: 4157.14 KB/s
Read block 3, bytes: 4096, speed: 4557.89 KB/s
Read block 4, bytes: 4096, speed: 8354.22 KB/s
Read block 5, bytes: 4096, speed: 6709.16 KB/s
Read block 6, bytes: 4096, speed: 7266.12 KB/s
Read block 7, bytes: 4096, speed: 8465.61 KB/s
Read block 8, bytes: 4096, speed: 6816.63 KB/s
Read block 9, bytes: 4096, speed: 8184.98 KB/s
File closed successfully.
```
**При использовании предзагрузки**
```
PS D:\Itmo\5_SEM\os-course\diy-page-cache-Miqvet\build\bin> .\test_prefetching.exe
Generating test file...
File generated: test_file.dat, size: 65536 bytes.
Testing cache prefetching...
Reading blocks sequentially...
[Prefetch] Starting prefetch from block 0 for 3 blocks
[Prefetch] Block 0 loaded into cache
[Prefetch] Block 1 loaded into cache
[Prefetch] Block 2 loaded into cache
[Prefetch] Prefetch completed
[Read] Block 0 loaded from disk
Read block 0, bytes: 4096, speed: 125.459 KB/s
[Read] Block 1 found in cache
Read block 1, bytes: 4096, speed: 12037.3 KB/s
[Read] Block 2 found in cache
Read block 2, bytes: 4096, speed: 11264.4 KB/s
[Prefetch] Starting prefetch from block 3 for 3 blocks
[Prefetch] Block 3 loaded into cache
[Prefetch] Block 4 loaded into cache
[Prefetch] Block 5 loaded into cache
[Prefetch] Prefetch completed
[Read] Block 3 loaded from disk
Read block 3, bytes: 4096, speed: 1151.31 KB/s
[Read] Block 4 found in cache
Read block 4, bytes: 4096, speed: 16666.7 KB/s
[Read] Block 5 found in cache
Read block 5, bytes: 4096, speed: 21254 KB/s
[Prefetch] Starting prefetch from block 6 for 3 blocks
[Prefetch] Block 6 loaded into cache
[Prefetch] Block 7 loaded into cache
[Prefetch] Block 8 loaded into cache
[Prefetch] Prefetch completed
[Read] Block 6 loaded from disk
Read block 6, bytes: 4096, speed: 1291.78 KB/s
[Read] Block 7 found in cache
Read block 7, bytes: 4096, speed: 25526.5 KB/s
[Read] Block 8 found in cache
Read block 8, bytes: 4096, speed: 36264.7 KB/s
[Prefetch] Starting prefetch from block 9 for 3 blocks
[Prefetch] Block 9 loaded into cache
[Prefetch] Block 10 loaded into cache
[Prefetch] Block 11 loaded into cache
[Prefetch] Prefetch completed
[Read] Block 9 loaded from disk
Read block 9, bytes: 4096, speed: 1466.92 KB/s
File closed successfully.
```

### Анализ:
#### Доп без предзагрузки:

1. При последовательном чтении данных каждый блок загружается по мере обращения к нему. Это приводит к постепенному увеличению скорости чтения.

#### Доп с предзагрузкой:

1. Предзагрузка позволяет заранее загрузить несколько блоков в кеш при чтении первого блока.
2. Результаты показывают, что предзагрузка значительно ускоряет чтение последующих блоков.

### Вывод:
В ходе лабораторной работы был реализован пользовательский кеш. Стандартный нагрузчик показывает лучшее время доступа к данным при первом чтении благодаря встроенному системному кешу. Однако пользовательский кеш способен обеспечить сопоставимую производительность при повторных запросах, эффективно используя RAM.

