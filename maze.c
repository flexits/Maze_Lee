
/****************************************************************************************************
*                               Федеральное агентство связи                                         *
*           Сибирский государственный университет телекоммуникаций и информатики                    *
*      Курсовая работа по дисциплине: "Технологии разработки программного обеспечения"              *
*                   Выполнил: Коростелин Александр Викторович, группа ПБТ-11                        *
*                                   Новосибирск, 2021                                               *
*****************************************************************************************************/
/****************************************************************************************************
*                                                                                                   *
*                    Программа maze.c - поиск кратчайшего пути в лабиринте.                         *
*                                                                                                   *
* В основе программы - алгоритм волнового поиска с ортогональным направлением (Lee algorithm).      *
* Нахождение пути выполняется в три стадии:                                                         *
* 1. Иницализация - создаётся массив ячеек plane[][] на основе данных, считанных из исходного файла;*
*   (функции verify_source(), load_array());                                                        *
* 2. Измерение расстояний - в отдельный массив marks[][] записывается метка каждой ячейки, численно *
* равная количеству шагов, которые необходимо пройти от стартовой ячейки до её достижения;          *
*   (функция mark_neighbours() и validate_point());                                                 *
* 3. Восстановление пути - на основе вычисленных отметок выстраивается путь прохождения лабиринта в *
* направлении от финишной ячейки до стартовой, двигаясь по точкам с наименьшими отметками;          *
*   (функция trace_back()).                                                                         *
*                                                                                                   *
*****************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <windows.h>
/*
 * Лабиринт описывается двумерным массивом с кодами в ячейках:
 * 0 - проход, 1 - стена, 2 - проход, начальная точка,
 * 3 - проход, конечная точка, 4 - проход, часть пути.
 */
#define PT_SPACE 0
#define PT_WALL 1
#define PT_ENTER 2
#define PT_EXIT 3
#define PT_PATH 4
/*
 * Максимальное и минимальное число ячеек лабиринта
 * по горизонтали и вертикали
 */
#define X_SIZE_MIN 4
#define X_SIZE_MAX 64
#define Y_SIZE_MIN 4
#define Y_SIZE_MAX 64
/*
 * Целочисленный массив plane[][] по умолчанию загружается из файла maze.csv
 */
#define SRC_FILE_NAME "maze.csv"
/*
 * Структура точки (ячейки) хранит её координаты в массиве
 * также эта структура используется для передачи размеров двумерных массивов в функции
 * (для сокращения числа передаваемых переменных)
 */
typedef struct Point {int x; int y;} Point;
/*
 * Проверяет корректность содержимого исходного файла и вычисляет размеры требуемого массива.
 * Возвращает false в случе ошибки.
 */
static bool verify_source(Point *plane_size, FILE *fptr);
/*
 * Заполняет ячейки массива значениями из файла. Возвращает false в случе ошибки.
 */
static bool load_array(const Point plane_size, int plane[plane_size.y][plane_size.x], FILE *fptr);
/*
 * Проверяет существование предполагаемой точки прохода pt (не является ли она стеной, не входит ли за границы лабиринта)
 */
static bool validate_point(const Point pt, const Point plane_size, const int plane[plane_size.y][plane_size.x]);
/*
 * Заносит в массив marks расстояние до всех точек прохода, граничащих с точкой pt
 * Функция рекурсивна, при вызове проходит сквозь весь лабиринт.
 * Если где-либо была встречена конечная точка PT_EXIT, возвращает true (выход достижим).
 */
static bool mark_neighbours(const Point pt, const Point plane_size, const int plane[plane_size.y][plane_size.x], int marks[plane_size.y][plane_size.x]);
/*
 * Рекурсивно восстанавливает путь сквозь лабириинт в обратном порядке - от конечной точки PT_EXIT до начальной PT_ENTER.
 */
static void trace_back(const Point ptexit, const Point plane_size, int plane[plane_size.y][plane_size.x], const int marks[plane_size.y][plane_size.x]);
/*
 * Выводит лабиринт на экран, отмечая найденный путь (стены - белые, путь - зелёный).
 * Выводит сообщение о том, что путь не найден, если solved == false
 */
static void maze_output(const Point plane_size, const int plane[plane_size.y][plane_size.x], const bool solved);
/*
 * Вывод заголовка программы (псевдографика)
*/
static void print_header(void);



static bool verify_source(Point *plane_size, FILE *fptr){
    if (fptr==NULL){
        printf("Ошибка открытия файла!\n");
        return false;
    }
    plane_size->x = 0; //счётчик столбцов
    plane_size->y = 0; //счётчик строк
    int c;
    //допустимая структура файла:
    //digit - separator - ... - newline
    //digit - separator - ... - separator - newline
    //digit - separator - ... - EOF
    //digit - separator - ... - separator - EOF
    //посимвольно считывая файл, проверяем соответствие структуры
    //одновреммно заполняем счётчики
    bool is_first_line = true;
    bool prev_char_was_digit = false;
    bool prev_char_was_newline = false;
    for(;;){
        c=fgetc(fptr);
        switch(c){
        case '0':
        case '1':
        case '2':
        case '3':
            if (prev_char_was_digit){
                printf("Неверное содержимое файла!\n(две цифры подряд)\n");
                return false;
            } else{
                prev_char_was_digit = true;
            }
            if (is_first_line){
                (plane_size->x)++; //каждая цифра в первой строке = столбец таблицы
            }
            prev_char_was_newline = false;
            break;
        case ',':
        case ';':
            if (prev_char_was_digit){
                prev_char_was_digit = false;
            } else{
                printf("Неверное содержимое файла!\n(пустая ячейка)\n");
                return false;
            }
            prev_char_was_newline = false;
            break;
        case '\n':
            if (!prev_char_was_newline){ //пропуская многократные переводы строки, увеличиваем счётчик
                (plane_size->y)++;
            }
            prev_char_was_newline = true;
            prev_char_was_digit = false;
            is_first_line = false;
            break;
        case EOF:
            if (!prev_char_was_newline){ //пропуская многократные переводы строки, увеличиваем счётчик
                (plane_size->y)++;
            }
            return true;
            break;
        default:
            printf("Неверное содержимое файла!\n(недопустимые символы)\n");
            return false;
        }
    }
    return true;
}

static bool load_array(const Point plane_size, int plane[plane_size.y][plane_size.x], FILE *fptr){
    if (fptr==NULL){
        printf("Ошибка открытия файла!\n");
        return false;
    }

    //проверяем соответствие размеров массива установленным ограничениям
    if (plane_size.x<X_SIZE_MIN || plane_size.x>X_SIZE_MAX || plane_size.y<Y_SIZE_MIN || plane_size.y>Y_SIZE_MAX){
        printf("Неверный размер лабиринта!\n");
        return false;
    }

    //посимвольно считываем файл, каждую цифру преобразовываем в int и записываем в очередной элемент массива
    //встретив цифры 2 и 3 (начало и конец пути), устанавливаем соответствующие флаги
    bool entrance_exists = false, exit_exists = false;
    int *element = &plane[0][0]; //указатель на очередной элемент массива
    int limiter = plane_size.x*plane_size.y; //общая длина массива
    int c;
    while ((c=fgetc(fptr))!=EOF && limiter>0){
        if (isdigit(c)){
            *element = atoi(&c);
            if (*element == PT_ENTER){
                entrance_exists = true;
            } else if (*element == PT_EXIT){
                exit_exists = true;
            }
            element++;
            limiter--;
        }
    }

    if (limiter>0){
        printf("Ошибка обработки ячеек!\n"); //файл закончился раньше, чем были считаны все данные = ошибка содержимого
        return false;
    }

    if (!entrance_exists){
        printf("Не найден вход в лабиринт!\n");
        return false;
    }

    if (!exit_exists){
        printf("Не найден выход из лабиринта!\n");
        return false;
    }

    return entrance_exists && exit_exists;
}

static bool validate_point(const Point pt, const Point plane_size, const int plane[plane_size.y][plane_size.x]){
    if (pt.x<0 || pt.x>=plane_size.x){
        return false; //за пределами лабиринта
    }
    if (pt.y<0 || pt.y>=plane_size.y){
        return false; //за пределами лабиринта
    }
    if (plane[pt.y][pt.x]==PT_WALL){
        return false; //нельзя пройти сквозь стену
    }
    return true;
}

static bool mark_neighbours(const Point pt, const Point plane_size, const int plane[plane_size.y][plane_size.x], int marks[plane_size.y][plane_size.x]){
    bool exit_reached = false; //флаг будет установлен если встречена конечная точка
    int distance = marks[pt.y][pt.x] + 1; //для соседней точки метка расстояния на 1 больше, чем для исходной
    //предполагаемая следующая точка ...
    Point pt_nxt = {pt.x,pt.y};
    int x_add[] = {+1, 0, -1, 0};
    int y_add[] = {0, +1, 0, -1};
    for (int i=0;i<4;i++){
        //... получается шагом в сторону (вверх, вниз, вправо, влево) ...
        pt_nxt.x = pt.x + x_add[i];
        pt_nxt.y = pt.y + y_add[i];
        //... если таковая точка пути существует ...
        if (validate_point(pt_nxt,plane_size,plane)){
            //... и не была посещена ранее, либо была посещена по более длинному пути ...
            if (marks[pt_nxt.y][pt_nxt.x] == -1 || marks[pt_nxt.y][pt_nxt.x] > distance){
                //... запишем метку в массив marks[][]
                marks[pt_nxt.y][pt_nxt.x] = distance;
                // рекурсивно вызываем функцию для каждой точки, сохраняя флаг
                if (mark_neighbours(pt_nxt,plane_size,plane,marks)){
                    exit_reached = true;
                }
            }
            if (plane[pt_nxt.y][pt_nxt.x]==PT_EXIT){
                exit_reached = true; //устанавливаем флаг, если достигнута конечная точка
                //return exit_reached; is it more effective?
            }
        }
    }
    return exit_reached;
}

static void trace_back(const Point pt, const Point plane_size, int plane[plane_size.y][plane_size.x], const int marks[plane_size.y][plane_size.x]){
    int temp_x, temp_y;
    Point pt_nxt = {pt.x,pt.y};
    //из четырёх соседних посещённых ранее точек находим ту, чья отметка наименьшая
    int x_add[] = {+1, 0, -1, 0};
    int y_add[] = {0, +1, 0, -1};
    for (int i=0;i<4;i++){
        temp_x = pt.x + x_add[i];
        temp_y = pt.y + y_add[i];
        if (marks[temp_y][temp_x] > 0 && marks[temp_y][temp_x] < marks[pt_nxt.y][pt_nxt.x]){
            pt_nxt.x = temp_x;
            pt_nxt.y = temp_y;
        }
    }
    //если такая точка найдена, отмечаем путь и рекурсивно вызываем функцию из неё
    if (pt_nxt.y != pt.y || pt_nxt.x != pt.x){
        plane[pt_nxt.y][pt_nxt.x] = PT_PATH;
        trace_back(pt_nxt,plane_size,plane,marks);
    }
}

static void maze_output(const Point plane_size, const int plane[plane_size.y][plane_size.x], const bool solved){
    //используем WinAPI для раскраски текста
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes; //сохраним дефолтные параметры консоли
    if (!solved){
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        printf("\tНе удалось пройти лабиринт - выход не достигнут!\n\n");
    }
    //выводим лабиринт по центру заголовка (80 символов)
    //отступ слева (80-2*plane_size.x)/2 = 40 - plane_size.x
    int indent = 40 - plane_size.x;
    for (int y=0; y<plane_size.y; y++){
        SetConsoleTextAttribute(hConsole, saved_attributes);
        printf("\t%*c", indent,' '); //отступ слева
        //для каждой точки в массиве plane[][] устанавливем цвет в зависимости от её типа
        for (int x=0; x<plane_size.x; x++){
            switch(plane[y][x]){
                case PT_WALL:
                    SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | FOREGROUND_INTENSITY);
                    break;
                case PT_ENTER:
                case PT_EXIT:
                    SetConsoleTextAttribute(hConsole, BACKGROUND_GREEN);
                    break;
                case PT_PATH:
                    SetConsoleTextAttribute(hConsole, BACKGROUND_GREEN | FOREGROUND_INTENSITY);
                    break;
                case PT_SPACE:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
                    break;
                default:
                    SetConsoleTextAttribute(hConsole, saved_attributes);
            }
            printf("%u ",plane[y][x]);
        }
        printf("\n");
    }
    SetConsoleTextAttribute(hConsole, saved_attributes); //восстановим дефолтные параметры
    printf("\n\tЛегенда: 0 - проход, 1 - стена, 2 - начало, 3 - конец, 4 - найденный путь\n");
}

static void print_header(void){
    printf("\
\t╔══════════════════════════════════════════════════════════════════════════════╗\n\
\t║                      Maze - нахождение пути в лабиринте                      ║\n\
\t╚══════════════════════════════════════════════════════════════════════════════╝\n\
\t                                                   Коростелин А.В., ПБТ-11, 2021\n\n");
}

int main(int argc, char *argv[])
{
    print_header();
    Point plane_size = {0,0}; //размер массива ячеек будет сохранён здесь

    //если имя файла передано аргументом при запуске, откроем его
    //иначе используем дефолтное имя
    char* filename = SRC_FILE_NAME;
    if (argc>1){
        filename = argv[1];
    }
    FILE *fptr = fopen(filename, "r");

    //валидация исходного файла и подсчёт размера массива
    if (!verify_source(&plane_size, fptr)){
        if (fptr){
            fclose(fptr);
        }
        return -1;
    }

    rewind(fptr);

    //подсчитав размер, объявляем идентичные по структуре массивы ячееек plane[][] и меток marks[][]
    //! первый индекс массива соответствует строке (y), второй - столбцу (x): array[y][x]
    int (*plane)[plane_size.x] = malloc(plane_size.y * plane_size.x * sizeof(plane[0][0]));
    for (int y=0; y<plane_size.y; y++){
        for (int x=0; x<plane_size.x; x++){
            plane[y][x] = PT_WALL;
        }
    }
    int (*marks)[plane_size.x] = malloc(plane_size.y * plane_size.x * sizeof(marks[0][0]));
    for (int y=0; y<plane_size.y; y++){
        for (int x=0; x<plane_size.x; x++){
            marks[y][x] = -1;
        }
    }

    //заполняем массив ячееек из файла
    if (!load_array(plane_size, plane, fptr)){
        if (fptr){
            fclose(fptr);
        }
        free(plane);
        free(marks);
        return -1;
    }

    fclose(fptr);

    //находим стартовую ячейку и запускаем функцию установки меток
    Point pt_start;
    bool exit_reachable = false; //флаг будет установлен при достижении конечной точки
    for (int x=0; x<plane_size.x; x++){
        for (int y=0; y<plane_size.y; y++){
            if (plane[y][x]==PT_ENTER){
                pt_start.x = x;
                pt_start.y = y;
                marks[y][x] = 0;
                if (mark_neighbours(pt_start,plane_size,plane,marks)){
                    exit_reachable = true;
                }
                break;
            }
        }
    }

    //если конец лабиринта найден, выстраиваем путь и выводим на экран
    if (exit_reachable){
        Point pt_end;
        for (int x=0; x<plane_size.x; x++){
            for (int y=0; y<plane_size.y; y++){
                if (plane[y][x]==PT_EXIT){
                    pt_end.x = x;
                    pt_end.y = y;
                    trace_back(pt_end,plane_size,plane,marks);
                    break;
                }
            }
        }
    }
    maze_output(plane_size,plane,exit_reachable);

    free(plane);
    free(marks);
    return 0;
}
