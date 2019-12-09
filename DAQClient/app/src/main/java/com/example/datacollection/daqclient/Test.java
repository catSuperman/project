package com.example.datacollection.daqclient;

import android.text.format.DateFormat;
import android.text.format.DateUtils;

import org.json.JSONException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class Test {
    public  static void  main(String[] args){

        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

        Date d = new Date();

        String dstr= d.toString();
        System.out.print(sdf.format(new Date(Date.parse(dstr))));
    }
}
