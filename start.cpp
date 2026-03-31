#define _CRT_SECURE_NO_WARNINGS
#include "constants.h"

#include <graphics.h>
#include <tchar.h>
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <cstdio>



// ------------------- Structures -------------------
typedef struct {
    int x, y, w, h;
    TCHAR text[16];
    COLORREF color;
} Button;   // структура для кнопки, координты и цвет 

typedef struct {
    int x, y, w, h;
} FieldRect;  // структура для поля ввода, координаты

// ------------------- Global data -------------------
static Button opBtns[5];  // создали массив из 5 кнопок для операций и выхода
static FieldRect field1, field2, fieldR;  // поля для первого операнда, второго операнда и результата

// текстовые буферы, которые хранят строки, отображаемые в полях ввода и вывода калькулятора.
static TCHAR buf1[32] = _T("");
static TCHAR buf2[32] = _T("");
static TCHAR bufR[64] = _T("");

// (активное поле для ввода, 0 - нет активного поля, 1 - первое поле, 2 - второе поле)
static int activeField = 0;  // 0 - none, 1 - field1, 2 - field2 

// ------------------- Helper functions -------------------
static bool in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

static void clear_result_on_edit() {
    bufR[0] = _T('\0');
}

static void set_result_error(const TCHAR* err) {
    _tcscpy(bufR, err);
}

// ------------------- Layout initialization -------------------
static void init_layout() {
    int startX = 20, startY = 20;
    const TCHAR* texts[5] = { _T("+"), _T("-"), _T("*"), _T("/"), _T("Exit") };
    COLORREF colors[5] = { BTN_COLOR, BTN_COLOR, BTN_COLOR, BTN_COLOR, EXIT_COLOR };

    for (int i = 0; i < 5; ++i) {
        opBtns[i].x = startX + i * (BTN_WIDTH + BTN_GAP);
        opBtns[i].y = startY;
        opBtns[i].w = BTN_WIDTH;
        opBtns[i].h = BTN_HEIGHT;
		// _tcscpy(opBtns[i].text, texts[i]) – копирует строку из texts[i] в поле 
		// text структуры кнопки. Это аналог strcpy, но для TCHAR
        _tcscpy(opBtns[i].text, texts[i]);
        opBtns[i].color = colors[i];
    }

    int fieldY = 120,  fieldX = 20;
    field1 = { fieldX, fieldY, FIELD_WIDTH, FIELD_HEIGHT};
    field2 = { fieldX + FIELD_WIDTH + FIELD_GAP, fieldY, FIELD_WIDTH, FIELD_HEIGHT };
    fieldR = { fieldX + 2 * (FIELD_WIDTH + FIELD_GAP), fieldY, FIELD_WIDTH, FIELD_HEIGHT };
}

// ------------------- Draw button -------------------
static void drawButton(const Button* btn) {
    setfillcolor(btn->color);
    solidrectangle(btn->x, btn->y, btn->x + btn->w, btn->y + btn->h);
    setlinecolor(RGB(0, 0, 0));
    rectangle(btn->x, btn->y, btn->x + btn->w, btn->y + btn->h);

    settextcolor(TEXT_COLOR);
    settextstyle(22, 0, _T("Consolas"));
    int tw = textwidth(btn->text);
    int th = textheight(btn->text);
    outtextxy(btn->x + (btn->w - tw) / 2,
              btn->y + (btn->h - th) / 2,
              btn->text);
}

// ------------------- Draw field -------------------
static void drawField(const FieldRect* f, const TCHAR* content, int isActive, int isReadOnly) {
    setfillcolor(FIELD_FILL_COLOR);
    solidrectangle(f->x, f->y, f->x + f->w, f->y + f->h);

    if (isReadOnly)
        setlinecolor(READ_FIELD_COLOR);
    else
        setlinecolor(isActive ? FIELD_BORDER_COLOR : RGB(0, 0, 0));
    rectangle(f->x, f->y, f->x + f->w, f->y + f->h);

    settextcolor(TEXT_COLOR);
    settextstyle(SIZE_TEXT, 0, _T("Consolas"));
    outtextxy(f->x + 8, f->y + 12, content);

    if (isActive && !isReadOnly) {
        int tx = f->x + 8 + textwidth(content);
        setlinecolor(FIELD_BORDER_COLOR);
        line(tx + 2, f->y + 8, tx + 2, f->y + f->h - 8);
    }
}

// ------------------- Full UI redraw -------------------
static void drawUI() {
    setbkcolor(BG_COLOR);
    cleardevice();

    setlinecolor(RGB(0, 0, 0));
    rectangle(MARGIN_WINDOW, MARGIN_WINDOW, WIN_WIDTH - MARGIN_WINDOW, WIN_HEIGHT - MARGIN_WINDOW);

    for (int i = 0; i < 5; ++i)
        drawButton(&opBtns[i]);

    settextcolor(TEXT_COLOR);
    settextstyle(SIZE_TEXT, 0, _T("Consolas"));
    outtextxy(field1.x, field1.y - MARGIN_FIELD_UP, _T("Operand 1:"));
    outtextxy(field2.x, field2.y - MARGIN_FIELD_UP, _T("Operand 2:"));
    outtextxy(fieldR.x, fieldR.y - MARGIN_FIELD_UP, _T("Result:"));

    drawField(&field1, buf1, (activeField == 1), false);
    drawField(&field2, buf2, (activeField == 2), false);
    drawField(&fieldR, bufR, false, true);

    settextcolor(RGB(128, 128, 128));
    settextstyle(14, 0, _T("Consolas"));
    outtextxy(10, WIN_HEIGHT - 25, _T("Click on field to edit, press operation button"));
}

// ------------------- Character input -------------------
static void append_char_to_active(TCHAR ch) {
    if (activeField != 1 && activeField != 2) return;

    TCHAR* target = (activeField == 1) ? buf1 : buf2;
    size_t len = _tcslen(target);
    if (len >= 31) return;

    if (ch == _T('-')) {
        if (len != 0) return;
        target[len] = ch;
        target[len + 1] = _T('\0');
        clear_result_on_edit();
        drawUI();
        return;
    }

    if (ch == _T('.')) {
        if (len == 0) return;
        for (size_t i = 0; i < len; ++i)
            if (target[i] == _T('.')) return;
        target[len] = ch;
        target[len + 1] = _T('\0');
        clear_result_on_edit();
        drawUI();
        return;
    }

    if (ch >= _T('0') && ch <= _T('9')) {
        target[len] = ch;
        target[len + 1] = _T('\0');
        clear_result_on_edit();
        drawUI();
        return;
    }
}

static void backspace_active() {
    if (activeField != 1 && activeField != 2) return;
    TCHAR* target = (activeField == 1) ? buf1 : buf2;
    size_t len = _tcslen(target);
    if (len > 0) {
        target[len - 1] = _T('\0');
        clear_result_on_edit();
        drawUI();
    }
}

// ------------------- Calculation -------------------
static void do_calc(TCHAR op) {
    if (_tcslen(buf1) == 0 || _tcslen(buf2) == 0) {
        set_result_error(_T("Error: enter both numbers"));
        drawUI();
        return;
    }

    double a = _tstof(buf1);
    double b = _tstof(buf2);
    double res = 0.0;
    bool ok = true;

    switch (op) {
        case _T('+'): res = a + b; break;
        case _T('-'): res = a - b; break;
        case _T('*'): res = a * b; break;
        case _T('/'):
            if (fabs(b) < 1e-12) {
                set_result_error(_T("Error: division by zero"));
                ok = false;
            } else {
                res = a / b;
            }
            break;
        default:
            set_result_error(_T("Error: unknown operation"));
            ok = false;
    }

    if (ok) {
        _stprintf(bufR, _T("%.6g"), res);
    }
    drawUI();
}

// ------------------- Mouse handling -------------------
static void handle_mouse_down(int mx, int my) {
    for (int i = 0; i < 5; ++i) {
        if (in_rect(mx, my, opBtns[i].x, opBtns[i].y, opBtns[i].w, opBtns[i].h)) {
            if (i == 4) {
                closegraph();
                exit(0);
            } else {
                do_calc(opBtns[i].text[0]);
            }
            return;
        }
    }

    if (in_rect(mx, my, field1.x, field1.y, field1.w, field1.h)) {
        activeField = 1;
    } else if (in_rect(mx, my, field2.x, field2.y, field2.w, field2.h)) {
        activeField = 2;
    } else {
        activeField = 0;
    }
    drawUI();
}

// ------------------- Main function -------------------
int main() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);
	/*
	Инициализирует расположение всех элементов интерфейса: вычисляет координаты и размеры для 
	пяти кнопок (opBtns[0]…opBtns[4]) и трёх полей (field1, field2, fieldR). Все позиции 
	задаются формулами (параметрическая вёрстка), чтобы легко менять внешний вид.
	*/
    init_layout();
	/*
	Первая полная отрисовка интерфейса: очищает окно, рисует внешнюю рамку, все 
	кнопки, подписи «Operand 1:», «Operand 2:», «Result:» и сами поля с текущим 
	содержимым буферов buf1, buf2, bufR. На старте все буферы пустые, поэтому поля пусты.
	*/
    drawUI();

	/*
	Объявляет переменную типа ExMessage — это структура EasyX, которая хранит информацию 
	о событии: тип сообщения, координаты мыши, код клавиши, символ и т.д. Будет 
	использоваться в цикле для получения данных от пользователя.
	*/
    ExMessage msg;

    while (true) {
		/*
		peekmessage(&msg, EM_MOUSE | EM_KEY | EM_CHAR) – неблокирующая функция, которая 
		проверяет, есть ли в очереди сообщения от мыши, клавиатуры (нажатия клавиш) или с
		имвольные сообщения.
		*/

        while (peekmessage(&msg, EM_MOUSE | EM_KEY | EM_CHAR)) {
			// WM_LBUTTONDOWN – сообщение о том, что левая кнопка мыши была нажата.
            if (msg.message == WM_LBUTTONDOWN) {
                /*
				Вызывается функция handle_mouse_down(msg.x, msg.y), которая получает координаты клика.
				Внутри неё проверяется, попал ли клик в одну из кнопок или в поля ввода. Если в кнопку 
				операции — выполняет вычисление; если в кнопку Exit — закрывает программу; если в поле 
				— меняет activeField (активное поле для ввода)
				*/
				handle_mouse_down(msg.x, msg.y);
            }
			// WM_CHAR – сообщение о вводе символа (буква, цифра, точка, backspace).
            else if (msg.message == WM_CHAR) {
                TCHAR ch = (TCHAR)msg.ch;
				// символ — backspace ('\b'), вызывается backspace_active(): удаляет 
				// последний символ из активного буфера (если активное поле — field1 или field2).
                if (ch == _T('\b')) {
                    backspace_active();
				// Иначе если символ печатаемый (код от 32 до 126, кроме 127 — это удалить нельзя), 
				// то вызывается append_char_to_active(ch): добавляет символ в активный буфер, очищает 
				// поле результата и перерисовывает интерфейс.
                } else if (ch >= 32 && ch != 127) {
                    append_char_to_active(ch);
                }
            }
			// WM_KEYDOWN – сообщение о нажатии клавиши (не символ). 
			// Используется для специальных клавиш, таких как Esc, Enter, стрелки.
            else if (msg.message == WM_KEYDOWN) {
                if (msg.vkcode == VK_ESCAPE) {
                    closegraph();
                    return 0;
                }
            }
        }
        Sleep(10);
    }

    closegraph();
    return 0;
}