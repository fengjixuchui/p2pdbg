package com.example.administrator.dbglibrary;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.LinkedList;

public class DbgActivity extends Activity implements View.OnClickListener {
    private static Context ms_ctx = null;

    /**
     * 调试用
     */
    private static boolean copyAssetsFile(String strAssets, String strDstPath) {
        try {
            InputStream from = ms_ctx.getAssets().open(strAssets);
            FileOutputStream to = new FileOutputStream(new File(strDstPath));

            byte buffer[] = new byte[4096];
            int iReadCount = 0;
            while (true) {
                iReadCount = from.read(buffer, 0, 4096);

                if (iReadCount <= 0) {
                    break;
                }
                to.write(buffer, 0, iReadCount);
            }
            from.close();
            to.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return true;
    }

    /**
     * copy插件目录中的插件到目标目录
     */
    private static boolean copyPluginFiles() {
        try {
            String dir = ms_ctx.getFilesDir() + "/test";
            new File(dir).mkdirs();

            String fileList[] = ms_ctx.getAssets().list("test");

            for (String file : fileList) {
                copyAssetsFile("test/" + file, dir + "/" + file);
            }
            return true;
        } catch (Exception e) {
        }
        return false;
    }

    /**
     * 调试用
     */
    protected void testFtp() {
        copyPluginFiles();

        String dir = ms_ctx.getFilesDir() + "/test";
        DbgLogic.sendFileToDbgger(new File(dir), "abcdef");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_dbg);
        m_textView = findViewById(R.id.text_msg);
        m_btnStop = findViewById(R.id.btn_debug);

        m_btnStop.setText("停止调试");
        m_btnStop.setOnClickListener(this);

        //TODO 调试接口
        /**
        ms_ctx = getBaseContext();
        DbgLogic.initDbgServ("abcd", getBaseContext());
        DbgLogic.registerCmd("testlog", "testlog", new DbgCmd.DbgCmdHandler() {
            @Override
            public LinkedList<String> onDbgCommand(String strCmd, String strParam) {
                testFtp();
                LinkedList<String> result = new LinkedList<String>();
                return result;
            }
        });
         */
        if (DbgLogic.isDbgServInit()) {
            m_textView.setText(DbgLogic.getUnique());
            new Thread() {
                public void run() {
                    DbgLogic.startDbgServ();
                }
            }.start();
        } else {
            m_textView.setText("尚未初始化 pid:" + android.os.Process.myPid());
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.btn_debug) {
            try {
                DbgLogic.stopDbgServ();
            } catch (Exception e) {
                String str = e.getMessage();
            }
            finish();
        }
    }

    private TextView m_textView = null;
    private Button m_btnStop = null;
}