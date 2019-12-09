package com.example.datacollection.daqclient;


/**
 * 信号数据结构
 */
public class ScSignal {
    /*信号类型*/
    private String signalType;

    /*信号编号*/
    private String signalNo;

    /*信号长度*/
    private int signalLen;

    /*是否有符号*/
    private boolean hasSignal;

    /*转换量*/
    private double transferValue;

    /*偏移量*/
    private double offsetValue;

    /*单位*/
    private String unit;

    /*是否实时显示*/
    private boolean isRtShow;

    /*英文名称*/
    private String englishName;

    /*中文名称*/
    private String chinesNmae;

    /*枚举组编号*/
    private String enumNo;

    /*原始数据*/
    private byte[] data;

    /*物理值*/
    private double physicalValue;

    public ScSignal(){ }

    public ScSignal(String signalType, String signalNo, int signalLen, boolean hasSignal, double transferValue,
                    double offsetValue, String unit, boolean isRtShow, String englishName, String chinesNmae, String enumNo){
        this.signalType = signalType;
        this.signalNo = signalNo;
        this.signalLen = signalLen;
        this.hasSignal = hasSignal;
        this.transferValue = transferValue;
        this.offsetValue = offsetValue;
        this.unit = unit;
        this.isRtShow = isRtShow;
        this.englishName = englishName;
        this.chinesNmae = chinesNmae;
        this.enumNo = enumNo;
    }

    public void setSignalType(String signalType){
        this.signalType = signalType;
    }

    public String getSignalType(){
        return signalType;
    }

    public void setSignalNo(String signalNo) {
        this.signalNo = signalNo;
    }

    public String getSignalNo() {
        return signalNo;
    }

    public void setSignalLen(int signalLen) {
        this.signalLen = signalLen;
    }

    public int getSignalLen() {
        return signalLen;
    }

    public void setHasSignal(boolean hasSignal) {
        this.hasSignal = hasSignal;
    }

    public boolean isHasSignal() {
        return hasSignal;
    }

    public void setTransferValue(double transferValue) {
        this.transferValue = transferValue;
    }

    public double getTransferValue() {
        return transferValue;
    }

    public void setOffsetValue(double offsetValue) {
        this.offsetValue = offsetValue;
    }

    public double getOffsetValue() {
        return offsetValue;
    }

    public void setRtShow(boolean rtShow) {
        isRtShow = rtShow;
    }

    public boolean isRtShow() {
        return isRtShow;
    }

    public void setUnit(String unit) {
        this.unit = unit;
    }

    public String getUnit() {
        return unit;
    }

    public void setEnglishName(String englishName) {
        this.englishName = englishName;
    }

    public String getEnglishName() {
        return englishName;
    }

    public void setChinesNmae(String chinesNmae) {
        this.chinesNmae = chinesNmae;
    }

    public String getChinesNmae() {
        return chinesNmae;
    }

    public void setEnumNo(String enumNo) {
        this.enumNo = enumNo;
    }

    public String getEnumNo() {
        return enumNo;
    }

    public void setData(byte[] data) {
        this.data = data;
    }

    public byte[] getData() {
        return data;
    }

    public double getPhysicalValue() {
        if(data == null){
            return  0;
        }
        if(signalLen < 1){
            return  0;
        }
        if(data.length != signalLen){
            return 0;
        }
        int temp = data[0]&0xFF;
        for(int i = 1; i <signalLen;i++){
            temp = (temp << 8)|(data[i]&0xFF);
        }
        if(hasSignal){
            if(signalLen == 2){
                temp = (short)temp;
            }
        }
        return temp * transferValue + offsetValue;
    }
}
