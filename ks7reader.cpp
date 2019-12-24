#include "ks7reader.h"



Q_LOGGING_CATEGORY(plcReader, "plcReader")

enum class reg_type //寄存器类型
{
    mBit = 0x01,
    mByte = 0x02,
    mWord = 0x04,
    mDouble = 0x06
};

KS7Reader::KS7Reader(QObject *parent) : QThread(parent)
{
    KInit();
}

KS7Reader::~KS7Reader()
{
    //销毁DB对象
    plcVarClear();
}

int KS7Reader::plcType2Length(QString fml_type)
{
    if( fml_type == tr("Bool")) {return 1;}
    else if( fml_type == tr("SInt")) {return 10;}
    else if( fml_type == tr("Int")) {return 20;}
    else if( fml_type == tr("DInt")) {return 40;}
    else if( fml_type == tr("Real")) {return 40;}
    else if( fml_type == tr("LReal")) {return 80;}

    return 0;
}

int KS7Reader::slot_connect()
{
    int ret = clt.ConnectTo(_ip.toStdString().c_str(), m_pack, m_slot);
    if(ret == 0) {
        emit connectedChanged();
        qCDebug(plcReader()) << tr("plc connected sucessfull");
    } else {
        qCDebug(plcReader()) << tr("plc connected failed");
    }

    return ret;
}

void KS7Reader::slot_disconnect()
{
    if(connected())
    {
        clt.Disconnect();
        emit connectedChanged();
        qCDebug(plcReader()) << tr("plc disconnected sucessfull");
    }
}

void KS7Reader::slot_startRead()
{
    start();
}

void KS7Reader::slot_stopRead()
{
    exit();
    qCDebug(plcReader()) << tr("waiting for thread quit...");
    wait();
    qCDebug(plcReader()) << tr("thread quit done");
}

void KS7Reader::KInit()
{
}

void KS7Reader::plcVarClear()
{
    qDeleteAll(plcVar.begin(), plcVar.end());
    plcVar.clear();
}

void KS7Reader::GenaretaMemMap()
{
    int curOffset = 0;  //当前变量的内存偏移地址
    //    int bnum = 0;  //连续的bool数量
    //    int bnum2 = 0;  //连续bool满16计数

    foreach (PlcDB* pdb, plcVar) {

        //计算上一个元素到当前的内存偏移
        pdb->offsets.append(0);
        for(int i=1; i<pdb->names.length();i++) {
            if(pdb->types[i] == tr("Bool")) {

            }
            else if(pdb->types[i] == tr("SInt")){

            }
            else if(pdb->types[i] == tr("Int")){

            }
            else if(pdb->types[i] == tr("Real")){

            }
            else if(pdb->types[i] == tr("LReal")){

            }
            pdb->offsets.append(curOffset);
        }

        qDebug() << pdb->offsets;
    }

}

void KS7Reader::slot_Req()
{
    //    ReqData(2, 0, 8);
    memset(m_reciveBuffer, 0, sizeof (m_reciveBuffer));

    //在这里实现自己的数据读取
    //各个数据需要 高低位反转. 存放bool的连续字节不需要反转
    int rret = clt.DBRead(2, 0, 54, &m_reciveBuffer);
    S7_DINT v1 = 0;
    reinterpret_cast<quint8*>(&v1)[0] = m_reciveBuffer[7];
    reinterpret_cast<quint8*>(&v1)[1] = m_reciveBuffer[6];
    reinterpret_cast<quint8*>(&v1)[2] = m_reciveBuffer[5];
    reinterpret_cast<quint8*>(&v1)[3] = m_reciveBuffer[4];
    //在这里实现自己的数据写入


    qDebug() << tr("read done");
}

//void KS7Reader::ReqData(int fml_db, int fml_stIndex, int fml_len)
//{

//    memset(_ctData, 0, sizeof(_ctData));
////    int ret = clt.DBRead(23, 0, 4, _ctData);
//    int ret = clt.DBRead(fml_db, fml_stIndex, fml_len, _ctData);
//    if(ret != 0)
//    {
//        cout<<"read error"<<endl;
//    }
//    else
//    {
//        quint8 temp[40] = {0};
//        memcpy(temp, _ctData, sizeof(_ctData));
//        //读取的数据需要进行高低位互换
//        if(!ret)
//        {
//            for(int i=0;i<10;i++)
//            {
//                S7_DINT byte1 = ((S7_DINT)temp[i*4])<<24;
//                S7_DINT byte2 = (S7_DINT)temp[i*4+1]<<16;
//                S7_DINT byte3 = ((S7_DINT)temp[i*4+2])<<8;
//                S7_DINT byte4 = (S7_DINT)temp[i*4+3];
//                _ctData[i] = byte1 + byte2 + byte3 + byte4;
//            }
//        }
//        cout<<"read val: "<<_ctData[0]<<endl;
//    }
//}


QVariantMap *KS7Reader::cpuInfo()
{
    return &m_cpuInfo;
}

bool KS7Reader::readCpuInfo()
{
    TS7CpuInfo cinfo = {};
    int ret = clt.GetCpuInfo(&cinfo);

    if(ret) {
        m_cpuInfo.clear();
        m_cpuInfo.insert(tr("ModuleTypeName"), tr(cinfo.ModuleName));
        m_cpuInfo.insert(tr("SerialNumber"), tr(cinfo.SerialNumber));
        m_cpuInfo.insert(tr("ASName"), tr(cinfo.ASName));
        m_cpuInfo.insert(tr("Copyright"), tr(cinfo.Copyright));
        m_cpuInfo.insert(tr("ModuleName"), tr(cinfo.ModuleName));

        emit cpuInfoChanged();
        return true;
    }
    return false;
}

void KS7Reader::setIp(QString fml_ip, short fml_slot)
{
    _ip = fml_ip;
    m_slot = fml_slot;
}

bool KS7Reader::loadDBInfo(QString fml_ffn)
{
    bool ret = false;
    QString curPath = QDir::currentPath();
    if( !QFile::exists(fml_ffn) ) return ret;

    plcVarClear();
    PlcDB* pdb = nullptr;


    //开始操作文件
    QFile file(fml_ffn);
    bool adb = false;  // a new db
    QTextStream ts(&file);
    QString sline;
    int dbLine = 0;
    if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) goto err1;

    while (!ts.atEnd()) {
        sline = ts.readLine();
        if(!adb && sline.indexOf("DATA_BLOCK \"") != -1) { //找到了首行定义
            adb = true;
            pdb = new PlcDB();
            plcVar.append(pdb);
            dbLine = 0;
        }
        if(adb && sline.indexOf("END_DATA_BLOCK") != -1) { //找到了DB定义结尾
            adb = false; }

        dbLine++;
        if(adb) {
            if(dbLine == 1) {  //获取db名称
                QRegExp reg("\"(\\w+)\"");  //DATA_BLOCK "Data_block_1"
                int pos = reg.indexIn(sline);
                if(pos != -1)  {
                    pdb->dbName = reg.cap(1);  //提取子元素
                    qCDebug(plcReader()) << "read db name: " << pdb->dbName;
                }
            }
            else if (dbLine == 2) {  //是否优化
                QRegExp reg("\'(\\w+)\'");  //{ S7_Optimized_Access := 'FALSE' }
                int pos = reg.indexIn(sline);
                if(pos != -1)  {
                    qDebug() << reg.capturedTexts();
                    pdb->s7_Optimized_Access = reg.cap(1) != "FALSE";
                    //                    qCDebug(plcReader()) << pdb->vName <<" s7_Optimized_Access" << pdb->s7_Optimized_Access;
                }
            }
            else if(dbLine >= 6) { //开始记录数据
                //检查是否到了声明尾部，优化快 结束是END_VAR 无优化块的结束是END_STRUCT
                if(sline.indexOf( pdb->s7_Optimized_Access ? "END_VAR" : "END_STRUCT") >= 0) { adb = false; }
                else {
                    //没有则继续按照变量读取
                    //(\w+) : (\w+)(\S+)?( \w+)* 备用，匹配任意声明
                    QRegExp reg("\\w+");  //匹配任意字幕、数字组成的短语,忽略空格及符号
                    int count = 0, pos = 0;
                    QStringList scols;
                    while ((pos = reg.indexIn(sline, pos)) != -1) {
                        ++count;
                        pos += reg.matchedLength();
                        scols.append(reg.capturedTexts()[0]);
                    }

                    //向DB添加数据，先判断是否是Array
                    pdb->names.append(scols[0]);
                    if(scols[1] == tr("Array")) {  //Array转换为QList
                        pdb->types.append(tr("Array[%1..%2] of %3").arg(scols[2]).arg(scols[3]).arg(scols[5]));
                        QVariantList array;
                        for(int i=0; i<scols[3].toInt()-scols[2].toInt()+1; i++) {
                            array.append(0);
                        }
                        pdb->values.append(array);
                    }else {
                        pdb->types.append(scols[1]);
                        pdb->values.append(0);  //给一个默认的值
                    }

                }

            }
        }
    }

    GenaretaMemMap();
    goto done;

err1:
    qCDebug(plcReader) << tr("error occurred when open file ") << fml_ffn;
    goto done;

done:
    //    qDebug() << pdb->offsets;
    return ret;
}

bool KS7Reader::loadDBInfo2(QString fml_ffn)
{
    bool ret = false;
    QString curPath = QDir::currentPath();
    if( !QFile::exists(fml_ffn) ) return ret;

    plcVarClear();
    PlcDB* pdb = new PlcDB();
    plcVar.append(pdb);

    //开始操作文件
    QFile file(fml_ffn);
    QTextStream ts(&file);
    QString sline;
    int dbLine = 0;
    int bnum = 0;
    if(!file.open(QIODevice::Text | QIODevice::ReadOnly)) goto err1;

    while (!ts.atEnd()) {
        sline = ts.readLine();

        if(sline.isEmpty()) continue;

        dbLine++;
        if(dbLine == 1) {  //获取db名称
            QRegExp reg("\"(\\w+)\"");  //DATA_BLOCK "Data_block_1"
            int pos = reg.indexIn(sline);
            if(pos != -1)  {
                pdb->dbName = reg.cap(1);  //提取子元素
                qCDebug(plcReader()) << "read db name: " << pdb->dbName;
            }
        }
        else if(dbLine >= 2) { //开始记录数据
            //没有则继续按照变量读取
            //(\w+) : (\w+)(\S+)?( \w+)* 备用，匹配任意声明
            QRegExp reg("\\w+");  //匹配任意字幕、数字组成的短语,忽略空格及符号
            int count = 0, pos = 0;
            QStringList scols;
            while ((pos = reg.indexIn(sline, pos)) != -1) {
                ++count;
                pos += reg.matchedLength();
                scols.append(reg.capturedTexts()[0]);
            }
            //向DB添加数据，先判断是否是Array
            pdb->names.append(scols[0]);
            if(scols[1] == tr("Array")) {  //Array转换为QList
                pdb->types.append(tr("Array[%1..%2] of %3").arg(scols[2]).arg(scols[3]).arg(scols[5]));
                QVariantList aryVal, aryOffset;
                int firstIndex = static_cast<int>(scols[6].toDouble() * 10);
                for(int i=0; i<scols[3].toInt()-scols[2].toInt()+1; i++) {
                    if(scols[5] == tr("Bool")) {
                        aryVal.append(QVariant(false));
                        aryOffset.append(firstIndex + bnum);
                        bnum++;
                        if(bnum > 7) {
                            firstIndex += 10;
                            bnum = 0;
                        }
                    } else {
                        aryVal.append(QVariant(0));
                        int tmp1 = i * plcType2Length(scols[5]);
                        aryOffset.append(firstIndex + i * plcType2Length(scols[5]));
                    }
                }
                pdb->offsets.append(aryOffset);
                pdb->values.append(aryVal);
            }else {
                pdb->types.append(scols[1]);
                pdb->values.append(0);  //给一个默认的值
                int offset = scols[2].toInt()*10 + scols[3].toInt();
                QVariantList aryOffset;
                aryOffset.append(offset);
                pdb->offsets.append(aryOffset);
            }

        }
    }

    GenaretaMemMap();
    goto done;

err1:
    qCDebug(plcReader) << tr("error occurred when open file ") << fml_ffn;
    goto done;

done:
//    qDebug() << pdb->names <<endl;
//    qDebug() << pdb->types <<endl;
//    qDebug() << pdb->offsets <<endl;
    return ret;
}

void KS7Reader::run()
{
    qDebug() << "thread id" << QThread::currentThreadId();
    ptmr.reset(new QTimer());
    connect(ptmr.data(), &QTimer::timeout, this, &KS7Reader::slot_Req);
    ptmr->start(m_reqInterval);
    exec();
}


bool KS7Reader::connected()
{
    return clt.Connected;
}

qint32 KS7Reader::reqInterval()
{
    return m_reqInterval;
}

void KS7Reader::setReqInterval(qint32 fml_interval)
{
    if(fml_interval < 1e10) {
        m_reqInterval = fml_interval;
    }
}

QVariant KS7Reader::readVariavle(QString fml_vName, int fml_index)
{
    return QVariant();
}

void KS7Reader::writeVariable(QString fml_vName, QVariant fml_value, int fml_index)
{

}


KS7Reader::PlcDB::~PlcDB()
{
    qCDebug(plcReader()) << "PlcDb release";
}

