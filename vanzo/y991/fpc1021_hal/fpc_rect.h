#ifndef FPC_RECT_H
#define FPC_RECT_H
namespace fpc {

class Rect {
public:
    Rect() : left(0), right(0),
        top(0), bottom(0) {}

    Rect(int left,int top, int right, int bottom) {
        this->left = left;
        this->bottom = bottom;
        this->top = top;
        this->right = right;
    }

    Rect(unsigned int width, unsigned int height) {
        this->left = 0;
        this->top = 0;
        this->right = width - 1;
        this->bottom = height - 1;
    }

    bool canContain(Rect& other) {
        return (other.left >= left && other.top >= top
                && other.right <= right && other.bottom <= bottom);
    }

    int width() {
        return right - left + 1;
    }

    int height() {
        return bottom - top + 1;
    }

    int left;
    int right;
    int top;
    int bottom;
};

} //namespace
#endif // FPC_RECT_H
