#include "ks7reader.h"



Q_LOGGING_CATEGORY(plcReader, "plcReader")

enum class reg_type //寄存器类型
{
    mBit = 0x01,
    mByte = 0x02,
    mWord = 0x04,
    mDouble = 0x06
};

KS7Reader::KS7Reader(QObject *parent) : QObject(parent)
{
    KInit();
}

int KS7Reader::slot_connect()
{
    int ret = clt.ConnectTo(_ip.toStdString().c_str(), m_pack, m_slot);
    if(ret == 0) emit connectedChanged();

    return ret;
}

void KS7Reader::slot_disconnect()
{
    if(connected())
    {
        clt.Disconnect();
        emit connectedChanged();
    }
}


void KS7Reader::KInit()
{
    //生成一个默认的DB信息，用于没有DB文件读取时的数据源
}

void KS7Reader::slot_Req()
{
    //    ReadTempture();
    //    ReqData(2, 0, 8);
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
////        printf("read val is: %08x\r\n", _ctData[0]);
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

    plcVar.clear();
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
                    pdb->vName = reg.cap(1);  //提取子元素
                    qCDebug(plcReader()) << "read db name: " << pdb->vName;
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

    goto done;

err1:
    qCDebug(plcReader) << tr("error occurred when open file ") << fml_ffn;
    goto done;

done:
    return ret;
}


bool KS7Reader::connected()
{
    return clt.Connected;
}

quint32 KS7Reader::reqInterval()
{
    return m_reqInterval;
}

void KS7Reader::setReqInterval(quint32 fml_interval)
{
    if(fml_interval < 1e10) {
        m_reqInterval = fml_interval;
    }
}

QVariant KS7Reader::readVariavle(QString fml_vName)
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
