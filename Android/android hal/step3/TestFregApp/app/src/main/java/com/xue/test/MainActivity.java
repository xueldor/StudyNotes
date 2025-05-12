package com.xue.test;

import android.app.Activity;
import android.freg.IFregService;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends Activity implements View.OnClickListener {

    private IFregService mFregService;

    private EditText regValEdit;
    private Button readButton;
    private Button writeButton;
    private Button clearButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mFregService = IFregService.Stub.asInterface(ServiceManager.getService("freg"));

        regValEdit = findViewById(R.id.edit_value);
        readButton = findViewById(R.id.button_read);
        writeButton = findViewById(R.id.button_write);
        clearButton = findViewById(R.id.button_clear);

        readButton.setOnClickListener(this);
        writeButton.setOnClickListener(this);
        clearButton.setOnClickListener(this);

        Log.i("freg", "onCreate");
    }

    @Override
    public void onClick(View v) {
        if(v.getId() == R.id.button_read){
            try {
                int val = mFregService.getVal();
                regValEdit.setText(String.valueOf(val));
            } catch (RemoteException e) {
                e.printStackTrace();
                Log.e("freg", "remote exception while reading from freg service");
            }
        }else if(v.getId() == R.id.button_write){
            String s = regValEdit.getText().toString();
            Log.i("freg", "try write" + s + " to freg");
            if(!TextUtils.isEmpty(s)) {
                try {
                    mFregService.setVal(Integer.parseInt(s));
                } catch (RemoteException e) {
                    e.printStackTrace();
                    Log.e("freg", "remote exception while writing to freg service");
                }
            }
        }else if(v.getId() == R.id.button_clear){
            regValEdit.setText("");
        }
    }
}