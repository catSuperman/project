package com.example.datacollection.daqclient;



import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

import io.reactivex.Observer;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.disposables.Disposable;
import io.reactivex.schedulers.Schedulers;

public class HistoryActivityT extends AppCompatActivity {

    // 表头
    private TableLayout tableHeadLayout;

    // 表体
    private TableLayout tableBodyLayout;

    private RadioGroup topButtonGroup;

    private static final int WC = TableRow.LayoutParams.WRAP_CONTENT;
    private static final int MP = TableRow.LayoutParams.MATCH_PARENT;

    // 表头字体颜色
    private int headTextColor;

    private Drawable bodyDivider;

    private static final  int ALL_DIVIDER = LinearLayout.SHOW_DIVIDER_BEGINNING|LinearLayout.SHOW_DIVIDER_MIDDLE|LinearLayout.SHOW_DIVIDER_END;

    private TableLayout.LayoutParams tableLayoutParam;

    private LinearLayout.LayoutParams btmlypara;

    private SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

    // 表头数据
    private List<HistoryActivityT.TableHead> headList = new ArrayList<>();

    private ProgressBar progressBar;

    private String curDevCode;

    private boolean isReCreateTable = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_history_t);

        tableHeadLayout = findViewById(R.id.table_head_layout);
        tableBodyLayout = findViewById(R.id.table_body_layout);
        progressBar = findViewById(R.id.progress_cycle);
        topButtonGroup = findViewById(R.id.top_radio_group);
        headTextColor = getResources().getColor(R.color.color2);
        bodyDivider = getResources().getDrawable(R.drawable.line_h, getTheme());
        tableLayoutParam = new TableLayout.LayoutParams(MP, WC, 1);
        btmlypara = new LinearLayout.LayoutParams(new ViewGroup.MarginLayoutParams(WC, WC));
        btmlypara.setMargins(2,5,5,5);
        Intent intent = getIntent();
        curDevCode = intent.getStringExtra("devCode");
        String[] tableNames = intent.getStringArrayExtra("tables");
        initTopButtons(tableNames);
        topButtonGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                String tabname = findViewById(i).getTag().toString();
                Log.i("TableTestActivity", "check_"+tabname);
                isReCreateTable = true;
                System.out.println("tabname :" + tabname);
                obserData(tabname);
            }
        });
    }

    /**
     * 根据表名数组动态生成查询相应表数据的按钮
     * @param tableNames
     */
    private void initTopButtons(String[] tableNames){
        System.out.println("tablename len : "+tableNames.length);
        if(tableNames == null || (tableNames.length < 1)){
            return;
        }
        topButtonGroup.removeAllViews();
        for(int i = 0; i < tableNames.length; i++){
            RadioButton btnItem = new RadioButton(topButtonGroup.getContext());
            btnItem.setText(tableNames[i].split("_")[0]);
            btnItem.setTag(tableNames[i]);
            btnItem.setChecked(false);
            btnItem.setTextSize(22);
            btnItem.setButtonDrawable(null);
            btnItem.setGravity(Gravity.CENTER);
            btnItem.setLayoutParams(btmlypara);
            btnItem.setBackgroundResource(R.drawable.radio);
            btnItem.setPadding(20,5,20,5);
            topButtonGroup.addView(btnItem);
            if(i == 0){
                topButtonGroup.check(btnItem.getId());
            }
        }
        //默认订阅第一个表的数据
        isReCreateTable = true;
        obserData(tableNames[0]);
    }

    /**
     * 订阅表的数据
     * @param tableName 表名
     */
    private void obserData(String tableName){
        if(tableName == null || tableName.isEmpty()){
            Toast.makeText(getApplicationContext(), "表名不合要求",Toast.LENGTH_SHORT).show();
            return;
        }
        if(isReCreateTable){
            tableHeadLayout.removeAllViews();
            tableBodyLayout.removeAllViews();
        }
        progressBar.setVisibility(View.INVISIBLE);
        HttpUtils.getHistoryDataByTabNameAndDevCode(tableName, curDevCode)
                .subscribeOn(Schedulers.newThread())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new Observer<JSONObject>() {
                    @Override
                    public void onSubscribe(Disposable d) { }

                    @Override
                    public void onNext(JSONObject jsonObject) {
                        if(isReCreateTable){
                            isReCreateTable = false;
                            createTableHead(jsonObject);
                        }
                        addBodyRow(jsonObject);
                    }
                    @Override
                    public void onError(Throwable e) {
                        Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
                    }
                    @Override
                    public void onComplete() {
                        progressBar.setVisibility(View.GONE);
                    }
                });
    }

    /**
     * 添加行
     * @param data
     * @return
     */
    private boolean addBodyRow(JSONObject data){
        if(data == null){
            return  false;
        }
        boolean flag = false;
        try{
            TableRow bodyRow = new TableRow(tableBodyLayout.getContext());
            bodyRow.setShowDividers(ALL_DIVIDER);
            bodyRow.setDividerDrawable(bodyDivider);
            bodyRow.setGravity(Gravity.CENTER);
            bodyRow.setOrientation(LinearLayout.HORIZONTAL);
            for(HistoryActivityT.TableHead head: headList){
                TextView bitem = new TextView(bodyRow.getContext());
                bitem.setTextAlignment(View.TEXT_ALIGNMENT_CENTER);
                bitem.setTextSize(22);
                bitem.setWidth(head.getWidth());
                if(head.getHeadName().equals("create_time")){
                    bitem.setText(sdf.format(new Date(data.getLong(head.getHeadName()))));
                }else {
                    bitem.setText(data.getString(head.getHeadName()));
                }
                bitem.setPadding(2,5,2,5);
                bodyRow.addView(bitem);
            }
            tableBodyLayout.addView(bodyRow, tableLayoutParam);
            flag = true;
        }catch (JSONException e){
            e.printStackTrace();
        }
        return flag;
    }

    /**
     * 根据第一条数据构建表头
     * @param firstData
     * @return
     */
    private boolean createTableHead(JSONObject firstData){
        boolean flag = false;
        headList.clear();
        try{
            Iterator<String> fiter = firstData.keys();
            // 表头顺序 id device_code .... create_time
            headList.add(new HistoryActivityT.TableHead("id", 4*52));
            headList.add(new HistoryActivityT.TableHead("device_code", 9*52));
            while (fiter.hasNext()){
                String key = fiter.next();
                if(key.equals("id") || key.equals("device_code") || key.equals("create_time")){
                    continue;
                }
                String value = firstData.getString(key);
                HistoryActivityT.TableHead tableHead = new HistoryActivityT.TableHead(key);
                // 计算列宽
                tableHead.setWidth(Math.max(key.length(), value.length())*52);
                headList.add(tableHead);
            }
            headList.add(new HistoryActivityT.TableHead("create_time", 13*52));
            // 生成表头
            tableHeadLayout.removeAllViews();
            TableRow headRow = new TableRow(tableHeadLayout.getContext());
            headRow.setShowDividers(LinearLayout.SHOW_DIVIDER_MIDDLE);
            headRow.setDividerDrawable(bodyDivider);
            headRow.setGravity(Gravity.CENTER);
            headRow.setOrientation(LinearLayout.HORIZONTAL);
            for (HistoryActivityT.TableHead thead: headList) {
                TextView titem = new TextView(headRow.getContext());
                titem.setWidth(thead.getWidth());
                titem.setText(thead.getHeadName());
                titem.setTextColor(headTextColor);
                titem.setTextSize(24);
                titem.setTextAlignment(View.TEXT_ALIGNMENT_CENTER);
                titem.getPaint().setFakeBoldText(true);
                titem.setPadding(2,15,2,15);
                headRow.addView(titem);
            }
            tableHeadLayout.addView(headRow, tableLayoutParam);
            flag = true;
        }catch (JSONException e){
            e.printStackTrace();
        }
        return  flag;
    }

    /**
     * 表头数据结构
     */
    public class TableHead{

        /*表名*/
        private String headName;

        /*列宽度*/
        private int width;

        public TableHead(){}

        public TableHead(String headName){
            this.headName = headName;
        }

        public TableHead(String headName, int width){
            this.headName = headName;
            this.width = width;
        }

        public void setHeadName(String headName){
            this.headName = headName;
        }

        public String getHeadName(){
            return headName;
        }

        public void setWidth(int width){
            this.width = width;
        }

        public int getWidth(){
            return width;
        }
    }
}
