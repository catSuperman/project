package com.example.datacollection.daqclient;


import android.support.annotation.NonNull;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.util.Collection;
import java.util.Iterator;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import io.reactivex.Observable;
import io.reactivex.ObservableEmitter;
import io.reactivex.ObservableOnSubscribe;
import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
public class TcpService {

    private Socket tcpClient = null;

    private String serveIp = "192.168.1.3";

    private int serverPort = 8080;

    private Thread  recvThread = null;

    private boolean isRun = false;

    private ScManager scManager;

    private static String errMsg = "";

    private Queue<JSONObject> recvQueue = new ConcurrentLinkedDeque<>();

    private final byte[] GET_ALL_ADEVCODES_BUF = {(byte) 0xFF, (byte) 0xFD, 0x64, 0x00, 0x00, 0x64};

    private byte[] GET_REALTIME_DATA_BUF = {(byte) 0xFF, (byte) 0xFD, 0x65, 0x00, 0x04,
            0x00, 0x00, 0x00, 0x00, 0x00};

    public TcpService(ScManager scManager, String serverIp, int serverPort){
        this.scManager = scManager;
        this.serveIp = serverIp;
        this.serverPort = serverPort;
    }

    public boolean  startService(){
        try{
            tcpClient = new Socket(this.serveIp, this.serverPort);
            tcpClient.setKeepAlive(true);
        }catch (Exception e){
            errMsg = e.getMessage();
            return false;
        }
        isRun = true;
        startRecvThread();
        return  true;
    }

    /**
     * 进行连接
     * @param ip
     * @param port
     * @return
     */
    public Observable<Boolean> connectServer(final String ip, final int port){
        errMsg = null;
        if(ip == null || ip.isEmpty() || (port < 1)){
            errMsg = "Tcp服务端Ip或Port不符合要求";
        }
        System.out.println("connect server");
        return  Observable.create(new ObservableOnSubscribe<Boolean>() {
            @Override
            public void subscribe(ObservableEmitter<Boolean> emitter) throws Exception {
                if(errMsg != null){
                    emitter.onError(new Error(errMsg));
                    emitter.onComplete();
                    return;
                }
                if(startService()){
                    emitter.onNext(true);
                }else{
                    emitter.onError(new Error(errMsg));
                    emitter.onNext(false);
                }
                emitter.onComplete();
            }
        });
    }

    /**
     * 发送获取所有在线设备编号的命令
     */
    public void getAllActiveDevCodes(){
        new Thread(){
            @Override
            public void run() {
                while (true){
                    if(tcpClient == null){
                        continue;
                    }
                    if(tcpClient.isInputShutdown()){
                        continue;
                    }
                    try{
                        tcpClient.getOutputStream().write(GET_ALL_ADEVCODES_BUF);
                    }catch (IOException e){
                        e.printStackTrace();
                    }
                    break;
                }
            }
        }.start();
        Log.i("TcpService", "获取在线设备编号列表");
    }

    public void startGetRealTimeDataByDevoce(String devCode){
        byte[] buf = devCode.getBytes();
        if(buf.length != 4){
            return;
        }
        for(int i = 0; i < 4; i++){
            GET_REALTIME_DATA_BUF[i+5] = buf[i];
        }
        new Thread(){
            @Override
            public void run() {
                byte crc = 0;
                for(int i = 2; i < 9; i++){
                    crc ^= GET_REALTIME_DATA_BUF[i];
                }
                GET_REALTIME_DATA_BUF[9] = crc;
                while (true){
                    if(tcpClient == null){
                        continue;
                    }
                    if(tcpClient.isInputShutdown()){
                        continue;
                    }
                    try{
                        tcpClient.getOutputStream().write(GET_REALTIME_DATA_BUF);
                    }catch (IOException e){
                        e.printStackTrace();
                    }
                    break;
                }
            }
        }.start();

        Log.i("TcpService", "请求获取设备实时数据: "+devCode);
    }

    private void startRecvThread(){
        recvThread = new Thread(){
            @Override
            public void run() {
                byte[] buf = new byte[1024];
                int len = 0;
                int index = 0;
                while (isRun){
                    if(tcpClient == null){
                        continue;
                    }
                    if(tcpClient.isInputShutdown()){
                        continue;
                    }
                    try {
                        len = tcpClient.getInputStream().read(buf);
                        index = 0;
                        while (len >= (index +6)){
                            // 判断帧头
                            if((buf[index]&0xFF) != 0xFF || (buf[index+1]&0xFF) != 0xFD){
                                index += 1;
                                Log.i("TcpService","帧头有误");
                                Log.i("TcpService", String.valueOf(buf[index]&0xFF));
                                Log.i("TcpService", String.valueOf(buf[index+1]&0xFF));
                                continue;
                            }
                            // 获取帧类型
                            int type = buf[index+2]&0xFF;
                            // 获取帧数据长度
                            int dlen = buf[index+3]&0xFF;
                            dlen = (dlen << 8) | (buf[index+4]&0xFF);
                            if((index+dlen) > len){
                                // 数据长度不够
                                Log.i("TcpService","数据长度不够");
                                break;
                            }
                            // 计算校验位
                            byte crc = 0;
                            for(int i = (index+2); i < (index+dlen+5); i++){
                                crc ^= buf[i];
                            }
                            if(crc != buf[index+dlen+5]){
                                index += (dlen+6);
                                Log.i("TcpService","校验有误：crc:" + crc + ": rc: " + buf[index+dlen+5]);
                                continue;
                            }
                            // 解析数据 一个设备标识为4bytes
                            JSONObject res = new JSONObject();
                            res.put("type", type);
                            if(dlen > 0){
                                if(type == 110){
                                    JSONArray array = new JSONArray();
                                    for(int i = (index+5); i < (index+5+dlen); i+= 4){
                                        char[] cs = {(char) buf[i], (char) buf[i+1], (char) buf[i+2], (char) buf[i+3]};
                                        array.put(String.copyValueOf(cs));
                                    }
                                    res.put("content", array);
                                }else if(type == 112){
                                    JSONArray array = new JSONArray();
                                    for(int i = (index+5); i < (index+5+dlen);){
                                        // 获取信号类型  信号长度 信号标识  原始数据
                                        String sigType = String.valueOf((buf[i]&0xF0)>>4);
                                        int sigLen = buf[i]&0x07;
                                        String sigNo = String.valueOf(buf[i+1]&0xFF);
                                        if((i+sigLen) > (index+5+dlen)){
                                            break;
                                        }
                                        byte[] data = new byte[sigLen];
                                        for(int j = 0; j <sigLen; j++){
                                            data[j] = buf[i+2+j];
                                        }
                                        ScSignal signal = scManager.procSignal(sigType, sigNo, data);
                                        if(signal != null){
                                            JSONObject obj = new JSONObject();
                                            obj.put(signal.getEnglishName(), signal.getPhysicalValue());
                                            array.put(obj);
                                        }
                                        i += (sigLen+2);
                                    }
                                    res.put("content", array);
                                }else{

                                }
                            }
                            recvQueue.add(res);
                            index += (dlen+6);
                        }
                    }catch (Exception e){
                        e.printStackTrace();

                    }
                    try {
                        Thread.sleep(50);
                    }catch (Exception e){
                        e.printStackTrace();
                    }
                }
            }
        };
        recvThread.start();
    }

    public Observable<JSONObject> getReadObservable(){
        return Observable.create(new ObservableOnSubscribe<JSONObject>() {
            @Override
            public void subscribe(final ObservableEmitter<JSONObject> emitter) throws Exception {
                Observable.interval(100, TimeUnit.MILLISECONDS).subscribe(new Observer<Long>() {
                    @Override
                    public void onSubscribe(Disposable d) { }

                    @Override
                    public void onNext(Long aLong) {
                        if (!recvQueue.isEmpty()){
                            emitter.onNext(recvQueue.poll());
                        }
                        if (!isRun){
                            emitter.onComplete();
                        }
                    }

                    @Override
                    public void onError(Throwable e) { }

                    @Override
                    public void onComplete() {  }
                });
            }
        });
    }

    public void stopService(){
        isRun = false;
        try {
            tcpClient.close();
        }catch (Exception e){
            e.printStackTrace();
        }
    }
}
