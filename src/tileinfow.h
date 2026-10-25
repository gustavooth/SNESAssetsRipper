#ifndef TILEINFOW_H
#define TILEINFOW_H

#include <QWidget>

namespace Ui {
class TileInfow;
}

class TileInfow : public QWidget
{
    Q_OBJECT

public:
    explicit TileInfow(QWidget *parent = 0);
    void     setTile(QPixmap tile, unsigned int setPos, unsigned int pcPos, unsigned int romPos, unsigned int dataPos);
    void     retranslate();
    ~TileInfow();

private:
    Ui::TileInfow *ui;
};

#endif // TILEINFOW_H
