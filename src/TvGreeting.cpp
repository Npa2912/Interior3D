#include "TvGreeting.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>

namespace {
void petal(QPainter &p, QPointF base, qreal angle, qreal width, qreal length, QColor color) {
    p.save();
    p.translate(base);
    p.rotate(angle);
    QPainterPath path;
    path.moveTo(0,0);
    path.cubicTo(-width*.8,-length*.32,-width*.62,-length*.96,-width*.18,-length*.97);
    path.cubicTo(width*.16,-length*1.1,width*.83,-length*.77,width*.53,-length*.38);
    path.quadTo(width*.22,-length*.1,0,0);
    QLinearGradient fill(0,0,0,-length);
    fill.setColorAt(0,color.darker(147));
    fill.setColorAt(.52,color);
    fill.setColorAt(1,color.lighter(139));
    p.fillPath(path,fill);
    p.restore();
}

void bouquet(QPainter &p) {
    const QPointF knot(825,390);
    const QPointF stems[]{{682,162},{720,106},{755,174},{792,107},{839,100},
                           {887,143},{925,113},{964,176},{722,220},{913,211}};
    p.setPen(QPen(QColor("#658967"),4,Qt::SolidLine,Qt::RoundCap));
    for(auto stem:stems) p.drawLine(knot,stem);
    for(int i=0;i<10;++i) {
        auto stem=stems[i];
        for(int j=0;j<4;++j) {
            QPointF leaf=stem+(knot-stem)*(.1+j*.115);
            petal(p,leaf,i%2?57+j*11:-64-j*9,22+j*2,42+j*4,
                  QColor::fromHsl(142+i%4*7,75+j*9,112+j*6));
        }
    }
    auto paper=[&](QPolygonF polygon,QColor top,QColor bottom) {
        QLinearGradient shade(710,166,930,432);
        shade.setColorAt(0,top);
        shade.setColorAt(1,bottom);
        p.setPen(Qt::NoPen);
        p.setBrush(shade);
        p.drawPolygon(polygon);
    };
    paper(QPolygonF({{652,196},{748,221},{815,368},{914,208},{973,186},{931,334},
                     {858,405},{875,465},{776,453},{795,397},{708,324}}),
          QColor("#fff3e1"),QColor("#b47b79"));
    paper(QPolygonF({{950,211},{890,297},{828,395},{903,450},{858,470},{790,426},
                     {768,385},{687,300},{657,227},{774,291},{831,328}}),
          QColor("#fce8d7"),QColor("#d99a98"));

    auto peony=[&](QPointF center,qreal size,QColor color) {
        for(int ring=0;ring<4;++ring) {
            const int count=10-ring;
            const qreal radius=size*(.99-ring*.19);
            for(int i=0;i<count;++i) {
                const qreal angle=i*360./count+ring*23;
                petal(p,center,angle,radius*.61,radius,color.lighter(92+ring*8+i%4*4));
            }
        }
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f5c28e"));
        for(int i=0;i<9;++i) {
            const qreal angle=i*2.39996;
            p.drawEllipse(center+QPointF(qCos(angle),qSin(angle))*(3.+i*.85),2,2);
        }
    };
    for(int i=0;i<14;++i) {
        QPointF center(i<7?690+i*4:910+(i-7)*5, i<7?120+i*9:97+(i-7)*10);
        petal(p,center, i%2?50:-56,8,16,QColor("#9778ba"));
    }
    peony({742,171},59,QColor("#df759e"));
    peony({892,208},62,QColor("#ec83aa"));
    auto lily=[&](QPointF center,qreal size,int angle) {
        for(int i=0;i<6;++i) petal(p,center,angle+i*60,size*.43,size,QColor("#fff5e3"));
        p.setPen(QPen(QColor("#c68b55"),2));
        for(int i=0;i<5;++i) {
            QPointF tip=center+QPointF(qCos(i*1.256),qSin(i*1.256))*size*.23;
            p.drawLine(center,tip);
            p.setBrush(QColor("#a45e37"));
            p.drawEllipse(tip,2.5,2.5);
        }
    };
    lily({855,136},49,13);
    lily({779,245},49,24);
    peony({824,224},63,QColor("#d96897"));

    QPainterPath ribbon;
    ribbon.moveTo(knot);
    ribbon.cubicTo(768,330,743,381,808,398);
    ribbon.cubicTo(759,404,775,430,750,451);
    ribbon.lineTo(782,449);
    ribbon.cubicTo(798,428,806,413,825,398);
    ribbon.cubicTo(860,421,854,444,885,455);
    ribbon.lineTo(892,434);
    ribbon.cubicTo(877,422,877,409,846,395);
    ribbon.cubicTo(921,378,865,342,825,390);
    QLinearGradient satin(772,370,878,422);
    satin.setColorAt(0,QColor("#af495c"));
    satin.setColorAt(.45,QColor("#f19699"));
    satin.setColorAt(1,QColor("#bd556b"));
    p.fillPath(ribbon,satin);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#eb9298"));
    p.drawRoundedRect(QRectF(814,384,24,18),5,5);
}
}

const QString &greetingText() {
    static const QString text=QStringLiteral("Chúc thầy Hiển\n20/11 vui vẻ");
    return text;
}

QImage greetingCard(int visibleCharacters) {
    QImage image(1024,536,QImage::Format_RGBA8888);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    QRadialGradient background(750,210,820);
    background.setColorAt(0,QColor("#283862"));
    background.setColorAt(1,QColor("#10182e"));
    p.fillRect(image.rect(),background);
    p.setPen(QPen(QColor("#b38f62"),1.5));
    p.drawRoundedRect(QRectF(24,24,976,488),16,16);
    p.setPen(Qt::NoPen);
    for(int i=0;i<49;++i) {
        const qreal x=42+(i*97)%937, y=45+(i*67)%445;
        if(x<633 && y>154 && y<355) continue;
        p.setBrush(QColor(247,207,142,70+(i%4)*28));
        p.drawEllipse(QPointF(x,y),1+i%3,1+i%3);
    }
    bouquet(p);
    const auto prefix=greetingText().left(visibleCharacters).split('\n');
    QFont font;
    font.setStyleHint(QFont::Serif);
    font.setPixelSize(64);
    font.setWeight(QFont::DemiBold);
    p.setFont(font);
    p.setPen(QColor("#fff0cf"));
    p.drawText(QRect(60,143,566,108),Qt::AlignVCenter|Qt::AlignLeft,prefix.value(0));
    font.setPixelSize(57);
    font.setWeight(QFont::Medium);
    p.setFont(font);
    p.drawText(QRect(61,231,560,110),Qt::AlignVCenter|Qt::AlignLeft,prefix.value(1));
    p.setPen(QPen(QColor("#d1a878"),2));
    p.drawLine(63,357,274,357);
    return image;
}
