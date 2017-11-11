#ifndef ZSTATVIEW_H
#define ZSTATVIEW_H

#include <QAbstractScrollArea>
#include <vector>

class ZAbstractStatModel;
class ZStatView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    ZStatView(QWidget *parent = nullptr);
    virtual ~ZStatView();

    void setModel(ZAbstractStatModel *model);
    void setMargins(int leftmargin, int topmargin, int rightmargin, int bottommargin);

    // Minimum spacing between two ticks on the vertical axis.
    int tickSpacing() const;
    // Set the minimum spacing between two ticks on the vertical axis. Can't be smaller than
    // the font metrics line height.
    void setTickSpacing(int val);

    // Scroll the statistics view to display the passed column at the center of the view.
    void scrollTo(int column);
protected:
    virtual void changeEvent(QEvent *event) override;
    virtual bool viewportEvent(QEvent *event) override;
    virtual void scrollContentsBy(int dx, int dy) override;
    virtual void paintEvent(QPaintEvent *event) override;
private:
    void paintBar(QPainter &p, int col, QRect r);

    // Rectangle used for displaying the statistics bars.
    QRect statRect() const;

    void updateView();

    ZAbstractStatModel *m;

    // Left, top, right and bottom margins left unpainted.
    int lm, tm, rm, bm;

    // Minimum space between two ticks on the vertical axis.
    int tickspacing;

    // Header text width left of the statistics area.
    int hwidth;

    // Text written on the vertical axis left to the statistics area.
    QString vlabel;
    // Text written on the horizontal axis above the statistics area.
    QString hlabel;

    // X coordinate of each column inside the view.
    std::vector<int> colpos;

    // Tick values and their pixel positions. The last item is the highest.
    std::vector<std::pair<int, int>> ticks;

    typedef QAbstractScrollArea base;
};

#endif // ZSTATVIEW_H
