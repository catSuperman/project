package com.example.datacollection.daqclient;


import android.content.SyncStatusObserver;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOError;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

/**
 * 信号解析器
 */
public class ScManager {
    /*配置文件版本号*/
    private String version;

    /*解析器状态*/
    private boolean isActive = false;

    /*信号类型编号和类型名称映射集合*/
    private Map<String, String> typesMap;

    /*枚举组编号和枚举组映射集合*/
    private Map<String, Map<Double,String>> enumGroupMap;

    /*信号类型编号和信号组映射集合*/
    private Map<String, Map<String, ScSignal>> signalsMap;

    private static ScManager scManagerInstance = new ScManager();

    private String filePath;

    private ScManager(){
        typesMap = new HashMap<String, String>();
        enumGroupMap = new HashMap<String, Map<Double, String>>();
        signalsMap = new HashMap<String, Map<String, ScSignal>>();
    }

    public static ScManager getScManagerInstance(){
        return scManagerInstance;
    }


    /**
     * 初始化解析器
     * @return
     */
    private boolean init(){
        InputStream is = null;
        BufferedReader br = null;
        try{
            is = new FileInputStream(this.filePath);
            br = new BufferedReader(new InputStreamReader(is, "GBK"));
        }catch(IOException e){
            e.printStackTrace();
            return false;
        }
        String str = null;
        String curType = null;
        String curIndex = null;
        while(true){
            try {
                str = br.readLine();
            }catch (IOException e){
                e.printStackTrace();
                return false;
            }
            if(str == null){
                break;
            }
            if(str.startsWith("\t"))
            {
                str = str.substring(1);
                if(curType.equals("SignalType")){
                    System.out.println(curType+"-"+curIndex+": " + str);
                    String[] signals = str.split(" ");
                    ScSignal signal = new ScSignal(curIndex, signals[0], Integer.valueOf(signals[1]),
                            signals[2].equals("1"), Double.parseDouble(signals[3]), Double.parseDouble(signals[4]),
                            signals[5], signals[6].equals("1"), signals[7], signals[8], signals[9]);
                    Map<String, ScSignal> signalMap = null;
                    if(this.signalsMap.containsKey(curIndex)){
                        signalMap = this.signalsMap.get(curIndex);
                    }else{
                        signalMap = new HashMap<String, ScSignal>();
                    }
                    signalMap.put(signals[0], signal);
                    this.signalsMap.put(curIndex, signalMap);
                    System.out.println(signal.toString());
                }else{
                    String[] kvsps = str.split(":");
                    if(curType.equals("SignalTypeMap")){
                        System.out.println(curType+": " + kvsps[0] + "-" + kvsps[1]);
                        this.typesMap.put(kvsps[0], kvsps[1]);
                    }
                    if(curType.equals("SignalEnumMap")){
                        System.out.println(curType+"-"+curIndex+": " + kvsps[0] + "-" + kvsps[1]);
                        Map<Double, String> enummap = null;
                        if(this.enumGroupMap.containsKey(curIndex)){
                            enummap = this.enumGroupMap.get(curIndex);
                        }else{
                            enummap = new HashMap<Double, String>();
                        }
                        enummap.put(Double.valueOf(kvsps[0]), kvsps[1]);
                        this.enumGroupMap.put(curIndex, enummap);
                    }
                }
            }else{
                if(str.indexOf(":") > 0){
                    String[] tsps = str.split(":");
                    curType = tsps[0];
                    curIndex = tsps[1];
                    if(curType.equals("version")){
                        this.version = curIndex;
                    }
                }else{
                    curType = str;
                }
            }
        }
        try {
            is.close();
            br.close();
        }catch (IOException e){
            e.printStackTrace();
        }
        this.isActive = true;
        return  true;
    }

    /**
     * 获取信号配置文件的版本号
     * @return
     */
    public String getVersion(){
        return version;
    }

    /**
     * 获取解析器的状态
     * @return
     */
    public  boolean isActive(){
        return  isActive;
    }

    /**
     * 启动信号管理器
     * @param filePath 信号配置文件路径
     * @return true: 文件存在正常启动; false: 文件有误无法启动
     */
    public boolean startManager(String filePath){
        File file = new File(filePath);
        if(!file.exists()){
            return  false;
        }
        this.isActive = false;
        this.filePath = filePath;
        new Thread(){
            @Override
            public void run() {
                init();
            }
        }.start();
        return true;
    }

    /**
     * 解析信号数据
     * @param typeNo  信号类型编号
     * @param signalNo  信号标识
     * @param data  原始数据
     * @return
     */
    public ScSignal procSignal(String typeNo, String signalNo, byte[] data){
        ScSignal signal = null;
        if(isActive)
        {
            if(this.signalsMap.containsKey(typeNo)){
                signal = this.signalsMap.get(typeNo).get(signalNo);
                if(signal == null){
                    System.out.println("不存在信号["+typeNo+"-"+signalNo+"]的解析器");
                }else{
                    signal.setData(data);
                }
            }else{
                System.out.println("不存在信号类型["+typeNo+"]的解析器");
            }
        }else{
            System.out.println("解析器不可用");
        }
        return  signal;
    }

    /**
     * 获取信号的枚举值
     * @param enumGroupNo 枚举组编号
     * @param value 信号物理值
     * @return
     */
    public String getSignalEnumValue(String enumGroupNo, Double value){
        String res = null;
        if(this.enumGroupMap.containsKey(enumGroupNo)){
            res = this.enumGroupMap.get(enumGroupNo).get(value);
        }
        return res;
    }

    /**
     * 获取所有信号类型随对应的数据表名
     * @return
     */
    public String[] getTableNames(){
        String[] tabNames = null;
        if(isActive){
            tabNames = new String[typesMap.size()];
            String ver = getVersion().replace(".", "_");
            int i = 0;
            for (String type: typesMap.values()) {
                tabNames[i] = type.toLowerCase()+"_"+ver;
                i++;
                if(i > tabNames.length){
                    break;
                }
            }
        }
        return tabNames;
    }
}
