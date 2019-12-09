package com.example.datacollection.daqclient;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SimpleAdapter;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import io.reactivex.Observer;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.schedulers.Schedulers;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    private static  final  String TAG = MainActivity.class.getSimpleName();


    // 服务器ip
    private final String SERVER_IP = "139.159.212.103";

    // TCP 服务器端口active_group
    private final int SERVER_PORT = 9001;

    // 信号配置文件路径
    private String signalFilePath;

    private Button btnConnect;

    private Button btnHttp;

    private TcpService tcpService;

    private boolean isConnected = false;

    // 当前显示实时数据的设备编号
    private String curDevCode;

    // 查看历史时间的设备编号
    private String searchDevCode;

    private ScManager scManager;

    private RadioGroup active_group;

    private GridView rtDataView;

    private  final int WRAP_CONTENT = LinearLayout.LayoutParams.WRAP_CONTENT;

    private LinearLayout.LayoutParams btm_lypara;

    // 实时数据集合
    private List<Map<String, String>> dataMaps;

    private SimpleAdapter adapter;

    private Button btnRefresh;

    private Button btnDownload;

    private Handler downLoadHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnConnect = findViewById(R.id.btnconnect);
        btnHttp = findViewById(R.id.btnhttp);
        rtDataView = findViewById(R.id.grid_view);
        btnRefresh = findViewById(R.id.btn_refresh);
        btnRefresh.setEnabled(false);
        btnDownload = findViewById(R.id.btn_download);

        btnConnect.setOnClickListener(this);
        btnHttp.setOnClickListener(this);
        btnRefresh.setOnClickListener(this);
        btnDownload.setOnClickListener(this);

        signalFilePath  = getApplicationContext().getFilesDir().getAbsolutePath()+"/test.sc";

        HttpUtils.setServerIp(SERVER_IP);

        scManager = ScManager.getScManagerInstance();
        if(!scManager.startManager(signalFilePath)){
            // 无法加载信号配置文件，需要先获取信号配置文件才能进行连接
            btnConnect.setEnabled(false);
        }
        tcpService = new TcpService(scManager, SERVER_IP, SERVER_PORT);

        active_group = findViewById(R.id.active_group);

        btm_lypara = new LinearLayout.LayoutParams(new ViewGroup.MarginLayoutParams(WRAP_CONTENT, WRAP_CONTENT));
        btm_lypara.setMargins(2,5,5,5);

        active_group.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                curDevCode = findViewById(i).getTag().toString();
                tcpService.startGetRealTimeDataByDevoce(curDevCode);
            }
        });
        dataMaps = new ArrayList<>();
        adapter = new SimpleAdapter(this,dataMaps, R.layout.gridview_item,
                new String[]{"label", "content"}, new int[]{R.id.label, R.id.content});
        rtDataView.setAdapter(adapter);
        downLoadHandler = new Handler(){
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what){
                    case 0x100:
                        Log.i(TAG, "start download");
                        break;
                    case 0x101:
                        Log.i(TAG, "download error");
                        break;
                    case 0x102:
                        Log.i(TAG, "download success");
                        tcpService.stopService();
                        reStartApp();
                        break;
                }
            }
        };
    }


    /**
     * 重启APP
     */
    public void reStartApp() {
        Intent intent = getBaseContext().getPackageManager().getLaunchIntentForPackage(getBaseContext().getPackageName());
        PendingIntent restartIntent = PendingIntent.getActivity(getApplicationContext(), 0, intent, PendingIntent.FLAG_ONE_SHOT);
        AlarmManager mgr = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
        mgr.set(AlarmManager.RTC, System.currentTimeMillis()+1000, restartIntent); // 1秒后重启应用
        System.exit(0);
    }

    @Override
    public void onClick(View view) {
        switch (view.getId())
        {
            case R.id.btnconnect:
                if(isConnected){
                    tcpService.stopService();
                    btnConnect.setText("连接");
                    isConnected = false;
                    btnRefresh.setEnabled(false);
                }else{
                    tcpService.connectServer(SERVER_IP, SERVER_PORT)
                            .subscribeOn(Schedulers.newThread())
                            .observeOn(AndroidSchedulers.mainThread())
                            .subscribe(new Observer<Boolean>() {
                                @Override
                                public void onSubscribe(Disposable d) { }

                                @Override
                                public void onNext(Boolean aBoolean) {
                                    if(aBoolean){
                                        isConnected = true;
                                        btnConnect.setText("断开");
                                        tcpService.getAllActiveDevCodes();
                                        observeRealTimeData();
                                        btnRefresh.setEnabled(true);
                                    }
                                }

                                @Override
                                public void onError(Throwable e) {
                                    Toast.makeText(getApplicationContext(),e.getMessage(), Toast.LENGTH_LONG).show();
                                }

                                @Override
                                public void onComplete() { }
                            });
                }
                break;
            case R.id.btnhttp:
                deviceCodeDialog(curDevCode);
                break;
            case R.id.btn_refresh:
                // 刷新 获取在线设备
                tcpService.getAllActiveDevCodes();
                break;
            case R.id.btn_download:
                btnConnect.setEnabled(false);
                btnRefresh.setEnabled(false);
                btnDownload.setEnabled(false);
                AsyncTask.execute(new DownloadConfig());
                break;
        }
    }

    /**
     * 跳转到历史记录页面
     * @param devCode
     */
    private void toHistoryActivity(String devCode){
        if(devCode == null || devCode.isEmpty()){
            Toast.makeText(getApplicationContext(), "设备编号不能为空", Toast.LENGTH_SHORT).show();
            return;
        }
        String[] tablesName = scManager.getTableNames();
        if (tablesName == null){
            Toast.makeText(getApplicationContext(), "获取表信息失败", Toast.LENGTH_LONG).show();
            return;
        }
        Intent intent = new Intent(this, HistoryActivityT.class);
        intent.putExtra("devCode", devCode);
        intent.putExtra("tables", tablesName);
        startActivity(intent);
    }

    /**
     * 设备编号确认弹窗
     * @param devCode
     */
    private void  deviceCodeDialog(final String devCode){
        final EditText inputText = new EditText(this);
        inputText.setText(devCode);
        inputText.setFocusable(true);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("请确认设备编码").setView(inputText)
                .setNegativeButton("取消", null)
                .setPositiveButton("确认", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        String deviceCode = inputText.getText().toString().trim();
                        toHistoryActivity(deviceCode);
                    }
                }).show();
    }

    /**
     * 下载信号配置文件
     */
    public class DownloadConfig implements Runnable{
        @Override
        public void run() {
            downLoadHandler.sendEmptyMessage(0x100);
            OutputStream output = null;
            try{
                URL url = new URL("http://"+SERVER_IP+":9999/sc_server/download");
                HttpURLConnection conn = (HttpURLConnection)url.openConnection();
                conn.setRequestMethod("GET");
                conn.setConnectTimeout(6*1000);
                if(conn.getResponseCode() == 200){
                    InputStream input = conn.getInputStream();
                    File file = new File(signalFilePath);
                    if(!file.exists()){
                        file.createNewFile();
                    }
                    output = new FileOutputStream(file);
                    byte[] buffer = new byte[1024];
                    int n = 0;
                    while ((n = input.read(buffer)) != -1){
                        output.write(buffer, 0, n);
                    }
                    output.flush();
                    input.close();
                }
            }catch (IOException e){
                e.printStackTrace();
                downLoadHandler.sendEmptyMessage(0x101);
            }finally {
                try{
                    output.close();
                }catch (IOException e){
                    e.printStackTrace();
                    downLoadHandler.sendEmptyMessage(0x101);
                }
            }
            downLoadHandler.sendEmptyMessage(0x102);
        }
    }

    /**
     * 获取实时数据
     */
    private void observeRealTimeData(){
        tcpService.getReadObservable().subscribeOn(Schedulers.newThread())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new Observer<JSONObject>() {
                    @Override
                    public void onSubscribe(Disposable d) {

                    }
                    @Override
                    public void onNext(JSONObject jsonObject) {
                        try{
                            int type = jsonObject.getInt("type");
                            System.out.println("type : "+type);
                            switch (type){
                                case 110:
                                    // 所有在线设备编号
                                    JSONArray array = jsonObject.getJSONArray("content");
                                    if(array == null){
                                        return;
                                    }
                                    String[] devCodes = new String[array.length()];
                                    for(int i = 0; i <array.length();i++){
                                        devCodes[i] = array.getString(i);
                                    }
                                    System.out.println("devCodes :" + devCodes);
                                    updateActiveDevBtns(devCodes);
                                    break;
                                case 111:
                                    // 所订阅设备断开连接
                                    Toast.makeText(getApplicationContext(), "设备断开连接", Toast.LENGTH_SHORT).show();
                                    break;
                                case 112:
                                    // 实时数据
                                    JSONArray rtDatas = jsonObject.getJSONArray("content");
                                    if(rtDatas != null && rtDatas.length() > 0){
                                        for(int i = 0; i < rtDatas.length(); i++){
                                            boolean isNotFind = true;
                                            JSONObject inData = rtDatas.getJSONObject(i);
                                            for (int j = 0; j < dataMaps.size(); j++){
                                                Map<String, String> dataMap = dataMaps.get(j);
                                                if(inData == null){
                                                    break;
                                                }
                                                String signalName = dataMap.get("label");
                                                System.out.println(signalName);
                                                if(!inData.isNull(signalName)){
                                                    isNotFind = false;
                                                    dataMap.put("content", inData.getString(signalName));
                                                    dataMaps.set(j, dataMap);
                                                    break;
                                                }
                                            }
                                            if(isNotFind){
                                                Map<String, String> newSignal = new HashMap<>();
                                                String signalName = inData.keys().next();
                                                newSignal.put("label", signalName);
                                                newSignal.put("content", inData.getString(signalName));
                                                dataMaps.add(newSignal);
                                            }
                                        }
                                        adapter.notifyDataSetChanged();
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }catch (JSONException e){
                            e.printStackTrace();
                        }
                    }

                    @Override
                    public void onError(Throwable e) {  }

                    @Override
                    public void onComplete() {
                        dataMaps.clear();
                        adapter.notifyDataSetChanged();
                        active_group.removeAllViews();
                    }
                });
    }

    /**
     * 更新底部在线设备按钮
     * @param devCodes
     */
    private void updateActiveDevBtns(String[] devCodes){
        active_group.removeAllViews();
        if(devCodes == null || (devCodes.length < 1)){
            return;
        }
        for (int i = 0; i < devCodes.length; i++){
            RadioButton devItem = new RadioButton(active_group.getContext());
            devItem.setText(devCodes[i]);
            devItem.setTag(devCodes[i]);
            devItem.setChecked(false);
            devItem.setButtonDrawable(null);
            devItem.setGravity(Gravity.CENTER);
            devItem.setLayoutParams(btm_lypara);
            devItem.setBackgroundResource(R.drawable.radio);
            active_group.addView(devItem);
        }
    }
}
