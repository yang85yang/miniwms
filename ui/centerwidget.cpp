﻿#include <assert.h>
#include "centerwidget.h"
#include "global.h"
#include "widgetgood.h"

CenterWidget::CenterWidget(QWidget *parent) : QWidget(parent)
{
}

void CenterWidget::init()
{
    //载入配置：
    //载入行数列数 货物A的行数
    row = configure.getValue("solution"+solutionPrefix+"/row").toInt();
    column = configure.getValue("solution"+solutionPrefix+"/column").toInt();
    rowA = configure.getValue("solution"+solutionPrefix+"/rowA").toInt();

    //初始化所有的按钮、货物
    for(int j=0;j<row;++j)
    {
        for(int i=0;i<column;++i)
        {
            int type = 1;
            if(j>=rowA)type=2;
            int id = i+j*column;
            if(type==2)id=i+(j-rowA)*column;
            int hasGood = configure.getValue(QString("solution"+solutionPrefix+"/hasGood/%1/%2").arg(j).arg(i)).toInt();
            WidgetGood *good = new WidgetGood(id+1,type,hasGood,solutionPrefix=="B");//方案B中，货物是旋转90°放置的
            widgetGoods.append(good);
        }
    }

    updateNext();

    centergroup = new QGroupBox(QStringLiteral("存放区"));

    //对货物的摆放方式进行设置
    initGoodPosition();

    QPushButton *takeABtn = new QPushButton(QStringLiteral("取走一个A"));
    QPushButton *takeBBtn = new QPushButton(QStringLiteral("取走一个B"));

    connect(takeABtn,SIGNAL(clicked(bool)),this,SLOT(onBtnA()));
    connect(takeBBtn,SIGNAL(clicked(bool)),this,SLOT(onBtnB()));

    QHBoxLayout *testTwoBtnHlayout = new QHBoxLayout;
    testTwoBtnHlayout->addStretch(1);
    testTwoBtnHlayout->addWidget(takeABtn);
    testTwoBtnHlayout->addStretch(1);
    testTwoBtnHlayout->addWidget(takeBBtn);
    testTwoBtnHlayout->addStretch(1);

    //显示累计值
    QLabel *todayAll = new QLabel(QStringLiteral("今天共计运送:"));
    QLabel *todayA = new QLabel(QStringLiteral("今天共计运送A(个):"));
    QLabel *todayB = new QLabel(QStringLiteral("今天共计运送B(个):"));

    tAll = new QLCDNumber;
    tAll->setDigitCount(5);
    tAll->display(0);
    // 设置显示外观样式
    tAll->setSegmentStyle(QLCDNumber::Flat);
    tAll->setStyleSheet("border: 0px; color: red; background: silver;");
    tAll->setMinimumHeight(40);

    tA = new QLCDNumber;
    tA->setDigitCount(5);
    tA->display(0);
    tA->setSegmentStyle(QLCDNumber::Flat);
    tA->setStyleSheet("border: 0px; color: red; background: silver;");
    tA->setMinimumHeight(40);


    tB = new QLCDNumber;
    tB->setDigitCount(5);
    tB->display(0);
    tB->setSegmentStyle(QLCDNumber::Flat);
    tB->setStyleSheet("border: 0px; color: red; background: silver;");
    tB->setMinimumHeight(40);

    updateNumberTimer.setInterval(5000);
    connect(&updateNumberTimer,SIGNAL(timeout()),this,SLOT(queryNumber()));

    QVBoxLayout *countlayout = new QVBoxLayout;
    countlayout->addStretch(1);
    countlayout->addWidget(todayAll);
    countlayout->addWidget(tAll);
    countlayout->addWidget(todayA);
    countlayout->addWidget(tA);
    countlayout->addWidget(todayB);
    countlayout->addWidget(tB);
    countlayout->addStretch(1);
    countlayout->setMargin(10);
    countlayout->setSpacing(10);


    //水平居中
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch(1);
    hlayout->addWidget(centergroup);
    hlayout->addItem(countlayout);
    hlayout->addStretch(1);

    //竖直居中
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addStretch(1);
    vlayout->addItem(testTwoBtnHlayout);
    vlayout->addItem(hlayout);
    vlayout->addStretch(3);

    setLayout(vlayout);

    updateNumberTimer.start();
}

void CenterWidget::queryNumber()
{
    int aMount = tA->value();
    int bMount = tB->value();
    QString nowTimeStr = QDateTime::currentDateTime().toString(DATE_TIME_FORMAT);
    QString todayStr = nowTimeStr.left(nowTimeStr.indexOf(" "));
    QString fromStr = todayStr + " 00:00:00";
    QString toStr = todayStr + " 23:59:59";

    QString sqlQuery = "select count(id) from agv_task where task_line=? and task_finishTime between ? and ?";
    QStringList params;
    params<<QString("%1").arg((int) Task::LineA)<<fromStr<<toStr;
    QList<QStringList> result = g_sql->query(sqlQuery,params);
    if(result.length()>0&&result.at(0).length()==1)
    {
        aMount = result.at(0).at(0).toInt();
        tA->display(aMount);
    }

    params.clear();
    params<<QString("%1").arg((int) Task::LineB)<<fromStr<<toStr;
    result = g_sql->query(sqlQuery,params);
    if(result.length()>0&&result.at(0).length()==1)
    {
        bMount = result.at(0).at(0).toInt();
        tB->display(bMount);
    }

    tAll->display(aMount+bMount);
}

//取走一个A货物
void CenterWidget::takeGoodA()
{
    if(nextIndexRowA==-1||nextIndexColumnA==-1)
    {
        QMessageBox::critical(this,QStringLiteral("错误"),QStringLiteral("当前位置无货物"),QMessageBox::Ok);
        return ;
    }

    //判断当前位置是否有货
    if(widgetGoods.at(nextIndexRowA*column+nextIndexColumnA)->hasGood()<=0)
    {
        QMessageBox::critical(this,QStringLiteral("错误"),QStringLiteral("当前位置无货物"),QMessageBox::Ok);
        return ;
    }

    //取走货物，对index进行下移
    widgetGoods.at(nextIndexRowA*column+nextIndexColumnA)->setHasGood(0);

    //保存配置文件中
    save();

    updateNext();
}

//取走一个B货物
void CenterWidget::takeGoodB()
{
    if(widgetGoods.at(nextIndexRowB*column+nextIndexColumnB)->hasGood()<=0)
    {
        QMessageBox::critical(this,QStringLiteral("错误"),QStringLiteral("当前位置无货物"),QMessageBox::Ok);
        return ;
    }

    //取走货物，对index进行下移
    widgetGoods.at(nextIndexRowB*column+nextIndexColumnB)->setHasGood(0);

    //保存配置文件中
    save();

    updateNext();
}

//保存状态到配置文件
void CenterWidget::save()
{
    //5、每个点位是否有货  hasgood
    for(int i=0;i<row;++i){
        for(int j=0;j<column;++j){
            configure.setValue(QString("solution"+solutionPrefix+"/hasGood/%1/%2").arg(i).arg(j),widgetGoods.at(i*column+j)->hasGood());
        }
    }
    //保存到配置文件
    configure.save();
}


void CenterWidget::onBtnA()
{
    controlCenter.onButtn(0x81);
}

void CenterWidget::onBtnB()
{
    controlCenter.onButtn(0x82);
}

void CenterWidget::updateNext()
{
    minA = 0;
    minB = 0;
    maxA = 0;
    maxB = 0;
    for(int i=0;i<column;++i)
    {
        for(int j=0;j<row;++j){
            int id = widgetGoods.at(j*column+i)->hasGood();
            if(j<rowA){
                if(id>0 && (id<minA || minA ==0)){
                    minA = id;
                    nextIndexColumnA = i;
                    nextIndexRowA = j;
                }
                if(id>maxA){
                    maxA = id;
                }
            }else{
                if(id>0 && (id<minB|| minB ==0)){
                    minB = id;
                    nextIndexColumnB = i;
                    nextIndexRowB = j;
                }
                if(id>maxB){
                    maxB = id;
                }
            }
        }
    }

    foreach (auto p, widgetGoods) {
        p->setFlicker(false);
    }

    if(minA<=0){
        nextIndexColumnA = -1;
        nextIndexRowA = -1;
    }else{
        widgetGoods.at(nextIndexRowA*column+nextIndexColumnA)->setFlicker(true);
    }

    if(minB<=0){
        nextIndexColumnB = -1;
        nextIndexRowB = -1;
    }else{
        widgetGoods.at(nextIndexRowB*column+nextIndexColumnB)->setFlicker(true);
    }
}

int CenterWidget::getNextAStation()
{
    if(nextIndexRowA*column+nextIndexColumnA > widgetGoods.length())return -1;
    if(widgetGoods.at(nextIndexRowA*column+nextIndexColumnA)->hasGood()>0)
    {
        return  nextIndexRowA*column+nextIndexColumnA;
    }
    return -1;
}

int CenterWidget::getNextBStation()
{
    if(nextIndexRowB*column+nextIndexColumnB > widgetGoods.length())return -1;
    if(widgetGoods.at(nextIndexRowB*column+nextIndexColumnB)->hasGood()>0)
    {
        return  nextIndexRowB*column+nextIndexColumnB;
    }
    return -1;
}


void CenterWidget::addGood(int add_row, int add_column)
{
    if(add_row<rowA){
        //A
        if(widgetGoods.at(add_row*column+add_column)->hasGood()>0)
        {
            QMessageBox::critical(NULL,QStringLiteral("错误"),QStringLiteral("该处已经存在货物"),QMessageBox::Ok);
        }else{
            widgetGoods.at(add_row*column+add_column)->setHasGood(++maxA);
            save();
            updateNext();
        }
    }else{
        //B
        if(widgetGoods.at(add_row*column+add_column)->hasGood()>0)
        {
            QMessageBox::critical(NULL,QStringLiteral("错误"),QStringLiteral("该处已经存在货物"),QMessageBox::Ok);
        }else{
            widgetGoods.at(add_row*column+add_column)->setHasGood(++maxB);
            save();
            updateNext();
        }
    }
}
