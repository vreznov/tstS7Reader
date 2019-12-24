#ifndef KS7READER_H
#define KS7READER_H

/*********************************************************************************************
 * class: KS7Reader
 * 功能：实现西门子PLC的读写操作。支持qml操作。
 *  仅支持db的数据读写，不支持 I Q M变量，需要使用这部分变量需要自行映射到DB块的变量
 * 作者：kare
 * 日期：2019.12.19
 * 版本：0.1
 *
 * 注意：
 * 1.西门子DB的内存分配，bool为bit单位，如果当前连续的bool不足一个WORD，按照WORD对齐
 * 使用方法
 * 1.从文件读取DB信息，在程序内解析并转换为 QVariableMap
 *
 *
 *
 *
 *
 ********************************************************************************************** */




#include <iostream>
#include "s7_client.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QVector>
#include <QVariantMap>
#include <QFile>
#include <QTextStream>
#include <QLoggingCategory>
#include <QRegExp>
#include <QSharedPointer>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QPointer>
#include <QSharedPointer>

//西门子plc对应变量类型定义
typedef quint16 S7_WORD;
typedef qint16 S7_INT;
typedef qint32 S7_DINT;
typedef float S7_REAL;

using namespace std;

Q_DECLARE_LOGGING_CATEGORY(plcReader)

// PLC读取
class KS7Reader : public QThread
{
    Q_OBJECT
//    Q_ENUMS()
public:
    explicit KS7Reader(QObject *parent = nullptr);
    ~KS7Reader();

    //! plc数据块定义
    struct PlcDB  {
        ~PlcDB();
        QString dbName;  //块名称
        QString dbIndex;  //块db编号
        bool s7_Optimized_Access = false;  //是否是优化的块结构，目前不支持优化结构
        //以下3个一一对应，同一个下标描述同一个变量
        QList<QString> names;
        QList<QString> types;
        QList<QVariant> values;
        QList<QVariant> offsets;  //db内的偏移地址。int类型.10代表1.0， 22.1代表第22字节的第2bit
        int dbSize = 0;  //db块的大小，bytes
    };

    //!给出plc对应类型的内存长度
    //! \brief plcType2Length
    //! \param fml_type
    //! \return
    //!
    int plcType2Length(QString fml_type);

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    bool connected(); //返回连接状态


    Q_PROPERTY(qint32 reqInterval READ reqInterval WRITE setReqInterval)
    qint32 reqInterval();
    void setReqInterval(qint32 fml_interval);


    Q_PROPERTY(QVariantMap* cpuInfo READ cpuInfo NOTIFY cpuInfoChanged)

    //!读取cpu信息，仅仅可用于S7300/400
    //! \brief readCpuInfo
    //!
    QVariantMap* cpuInfo();

    //!从plc读取cpu信息
    //! \brief readCpuInfo
    //! \return 是否成功读取
    Q_INVOKABLE bool readCpuInfo();


    //!读取plc变量，从当前内存读取，不是执行去plc读取的动作
    //! \brief readVariavle
    //! \param fml_vName 变量名称
    //! \return 变量结果
    //!
    Q_INVOKABLE  QVariant readVariavle(QString fml_vName, int fml_index = -1);

    //!写plc变量，操作内存数据
    //! \brief writeVariable
    //! \param fml_vName 变量名称
    //! \param fml_value 变量值
    //! \param fml_index 如果变量是一个索引，针对索引的某个下标进行写入，只支持一维数组
    //!
    Q_INVOKABLE  void writeVariable(QString fml_vName, QVariant fml_value, int fml_index = -1);


    //!设置IP 槽号
    //! \brief setIp
    //! \param fml_ip
    //! \param fml_slot
    //!
    Q_INVOKABLE void setIp(QString fml_ip, short fml_slot=1);


    //!从文件读取DB信息 功能不完善
    //! \brief readDBInfo
    //! \param fml_ffn db导出的源文件，可以将多个DB导出为一个文件
    //! \return
    //!
    Q_INVOKABLE bool loadDBInfo(QString fml_ffn);

    //!从自定义的db文件中读入db信息
    //! \brief loadDBInfo2
    //! \param fml_ffn
    //! \return
    //!
    bool loadDBInfo2(QString fml_ffn);

    void run() override;
private:
    void KInit();

    //! 与PLC的通信线程
    QThread reqThread;

    //======属性对应变量==================
    qint32 m_reqInterval = 100;  //ms单位
    QVariantMap m_cpuInfo;  //存储读取到的cpu信息

    //! plc变量
    //! 每一个QVariantMap代表一个DB数据块
    QList<PlcDB*> plcVar;

    //!销毁plcvar的指针对象
    //! \brief plcVarClear
    //!
    void plcVarClear();

    //! 从PLC读取数据
    //! @param fml_db: 要读取的 DB编号
    //! @param fml_stIndex :索引起始地址
    //! @param fml_len: 数据字节长度
    void ReqData(int fml_db = 0, int fml_stIndex = 0, int fml_len = 4); //读取计数数据

    TSnap7Client clt; //PLC client对象

    QString _ip= "192.168.0.10";
    short m_pack = 0; //机架0
    short m_slot = 1; //s7-1200 槽号1. s7-300槽号2

    quint8 m_reciveBuffer[1024] = {};  //接收plc数据的缓冲区，1024字节
    quint8 m_sendBuffer[1024] = {};  //发送给plc的数据的缓冲区，1024字节

    //!生成plc变量对应的内存偏移分配，根据当前已经读取的DB来执行
    //! \brief GenaretaMemMap
    //!
    void GenaretaMemMap();

    QSharedPointer<QTimer> ptmr;

signals:
    void connectedChanged();  //与plc的连接状态已变更
    void cpuInfoChanged();  //新cpu信息已读取

public slots:

    //!执行plc通信的函数，可以由线程的定时器连接或独立的定时器连接
    //! \brief slot_Req
    //!
    void slot_Req(); //查询数据

    int slot_connect(); //连接到PLC
    void slot_disconnect(); //断开连接
    void slot_startRead();
    void slot_stopRead();

};


#endif // S7_READER_H
