package com.example.administrator.dbglibrary;

import android.os.SystemClock;
import android.util.Log;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;

public class DbgServ {
    interface DbgServHandler {
        void onConnect(Socket sock);
        void onRecvData(Socket sock, byte[] data);
    };

    //TODO 接收服务端法颂过来的数据
    class ClientThread extends Thread {
        public void run() {
            byte buffer[] = new byte[4096];
            while (true) {
                try {
                    while (true) {
                        InputStream in = m_clientSocket.getInputStream();
                        int size = in.read(buffer, 0, 4096);

                        if (m_msgHandler != null && size > 0) {
                            byte userData[] = new byte[size];
                            System.arraycopy(buffer, 0, userData, 0, size);
                            m_msgHandler.onRecvData(m_clientSocket, userData);
                        }
                    }
                } catch (Exception e) {
                    if (m_bStopWork) {
                        break;
                    }
                    SystemClock.sleep(1000);
                }
            }
        }
    };

    private boolean isSocketAlive(Socket sock) {
        if (null == sock || sock.isClosed() || !sock.isConnected()) {
            return false;
        }
        return true;
    }

    private void testConnect() {
        try {
            if (!isSocketAlive(m_clientSocket)) {
                m_clientSocket = new Socket();
                m_clientSocket.connect(m_msgAddress, 3000);

                if (m_clientSocket.isConnected()) {
                    m_strLocalIp = m_clientSocket.getLocalAddress().getHostAddress();
                    //sock.setKeepAlive(true);
                    m_msgHandler.onConnect(m_clientSocket);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    class KeepAliveThread extends Thread {
        public void run() {
            while (true) {
                try {
                    synchronized (m_notifyStop) {
                        m_notifyStop.wait(5000);
                    }

                    if (m_bStopWork) {
                        break;
                    }
                    testConnect();
                } catch (Exception e) {
                    Log.e("DbgLib", e.getMessage());
                }
            }
        }
    };

    public String getLocalIp() {
        return m_strLocalIp;
    }

    public boolean stopDbgServ() {
        try {
            m_bStopWork = true;
            synchronized (m_notifyStop) {
                m_notifyStop.notifyAll();
            }

            if (m_clientSocket != null) {
                m_clientSocket.close();
            }

            if (m_servSocket != null) {
                m_servSocket.close();
            }

            m_hMsgThread.join();
            m_hKeepAlive.join();

            m_clientSocket = null;
            m_servSocket = null;
        } catch (Exception e) {
        }
        m_bConnectSucc = false;
        return true;
    }

    public boolean initDbgServ(String strServIp, int iMsgPort, int iFtpPort, int iLocalPort, DbgServHandler msgHandler, DbgServHandler ftpHandler) {
        if (m_bInit) {
            return true;
        }

        try {
            m_bInit = true;
            m_bStopWork = false;
            m_msgHandler = msgHandler;
            m_ftpHandler = ftpHandler;

            m_msgAddress = new InetSocketAddress(strServIp, iMsgPort);
            m_ftpAddress = new InetSocketAddress(strServIp, iFtpPort);
            m_clientSocket = new Socket();

            testConnect();
            m_hMsgThread = new ClientThread();
            m_hKeepAlive = new KeepAliveThread();
            m_hMsgThread.start();
            m_hKeepAlive.start();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
        }
        return false;
    }

    public boolean send(Socket sock, byte[] userData) {
        try {
            if (sock != null) {
                OutputStream out = sock.getOutputStream();
                out.write(userData);
                out.flush();
                return true;
            }
        } catch (Exception e) {
            try {
                sock.close();
            } catch (Exception f) {
            }
            Log.e("DbgServ", "err:" + e.getMessage());
        }
        return false;
    }

    /**
     * 获取ftp套接字,使用短连接进行ftp传输
     */
    public Socket getFtpClient() {
        boolean bResult = false;
        Socket sock = new Socket();
        try {
            sock.connect(m_ftpAddress, 3000);

            if (sock.isConnected()) {
                bResult = true;
                return sock;
            } else {
                return null;
            }
        } catch (Exception e) {
            return null;
        } finally {
            if (!bResult) {
                try {
                    sock.close();
                } catch (Exception e) {
                }
            }
        }
    }

    /**
     * 发送至公网服务器
     */
    public boolean sendToServ(byte[] userData) {
        return send(m_clientSocket, userData);
    }

    /**
     * 0 直连模式
     * 1 转发模式
     */
    private DbgServHandler m_msgHandler = null;
    private DbgServHandler m_ftpHandler = null;
    private ServerSocket m_servSocket = null;
    private Socket m_acceptClient = null;       //直连过来的调试socket
    private Socket m_clientSocket = null;       //连接服务器的socket

    private String m_strLocalIp = null;         //本地ip地址
    private boolean m_bConnectSucc = false;     //是否网络连接正常
    private boolean m_bInit = false;
    private InetSocketAddress m_msgAddress = null;
    private InetSocketAddress m_ftpAddress = null;
    private Thread m_hMsgThread = null;
    private Thread m_hKeepAlive = null;
    private Object m_notifyStop = new Object();
    private boolean m_bStopWork = false;
}
