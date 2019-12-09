package com.example.datacollection.daqclient;


import android.util.Log;

import org.jetbrains.annotations.NotNull;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONStringer;

import java.io.IOException;
import java.util.Date;

import io.reactivex.Observable;
import io.reactivex.ObservableEmitter;
import io.reactivex.ObservableOnSubscribe;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;


/**
 * http服务工具类
 */
public class HttpUtils {
    private static OkHttpClient hclient = new OkHttpClient();

    private static String SERVER_IP = "192.168.1.3";

    public static Observable<JSONObject> getHistoryDataByTabNameAndDevCode(final String tabname, final String devCode){
        return Observable.create(new ObservableOnSubscribe<JSONObject>() {
            @Override
            public void subscribe(final ObservableEmitter<JSONObject> emitter) throws Exception {
                Request request = new Request.Builder().url("http://"+SERVER_IP+":9999/sc_server/"+tabname+"/"+devCode).build();
                hclient.newCall(request).enqueue(new Callback() {
                    @Override
                    public void onFailure(@NotNull Call call, @NotNull IOException e) {
                        emitter.onError(e);
                    }

                    @Override
                    public void onResponse(@NotNull Call call, @NotNull Response response) throws IOException {
                        if(response.isSuccessful()){
                            JSONObject obj = null;
                            try{
                                obj = new JSONObject(response.body().string());
                            }catch (JSONException e){
                                emitter.onError(e);
                            }
                            if(obj != null){
                                try{
                                    boolean flag = obj.getBoolean("flag");
                                    if(flag){
                                        JSONArray array = obj.getJSONArray("content");
                                        if(array != null && (array.length() > 0)){
                                            for(int i = 0; i < array.length(); i++){
                                                emitter.onNext(array.getJSONObject(i));
                                            }
                                        }
                                    }
                                }catch (JSONException e){
                                    emitter.onError(e);
                                }
                            }
                            emitter.onComplete();
                        }
                    }
                });
            }
        });
    }

    public static void setServerIp(String ip){
        SERVER_IP = ip;
    }


}
