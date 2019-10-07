package com.example.administrator.dbglibrary;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * 文件/文件夹压缩工具类 lougd
 */
public class DbgCompress {
    private static boolean zipFileInternal(ZipOutputStream zipOut, File file, String strBaseDir) {
        try {
            if (file.isDirectory()) {
                File files[] = file.listFiles();

                for (File fp : files) {
                    if (fp.isDirectory()) {
                        strBaseDir = file.getName() + File.separator + fp.getName() + File.separator;
                        zipFileInternal(zipOut, fp, strBaseDir);
                    } else {
                        zipFileInternal(zipOut, fp, strBaseDir);
                    }
                }
            } else {
                byte buffer[] = new byte[4096];
                InputStream input = new BufferedInputStream(new FileInputStream(file));
                zipOut.putNextEntry(new ZipEntry(strBaseDir + file.getName()));

                int iReadSize = 0;
                while ((iReadSize = input.read(buffer)) > 0) {
                    zipOut.write(buffer, 0, iReadSize);
                }
                input.close();
            }
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    private static void createSubFolders(String strFileName, String path) {
        String[] subFolders = strFileName.split("/");
        if (subFolders.length <= 1) {
            return;
        }

        String pathNow = path;
        for (int i = 0; i < subFolders.length - 1; ++i) {
            pathNow = pathNow + subFolders[i] + "/";
            File fmd = new File(pathNow);
            if (fmd.exists()) {
                continue;
            }
            fmd.mkdirs();
        }
    }

    private static String createSeparator(String path) {
        File dir = new File(path);
        if (!dir.exists()) {
            dir.mkdirs();
        }
        if (path.endsWith("/")) {
            return path;
        }
        return path + '/';
    }

    /**
     * 压缩文件或者文件夹
     */
    public static boolean zipFile(File file, String strZipPath) {
        ZipOutputStream zipOut = null;
        boolean bStat = false;

        try {
            zipOut = new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(strZipPath)));
            if (file.isDirectory()) {
                zipFileInternal(zipOut, file, file.getName() + File.separator);
            } else {
                zipFileInternal(zipOut, file, "");
            }
            bStat = true;
        } catch (Exception e) {
            bStat = false;
        } finally {
            try {
                zipOut.closeEntry();
                zipOut.close();
            } catch (Exception e) {
            }
        }
        return bStat;
    }

    /**
     * 解压缩文件或者文件夹
     */
    public static boolean unZipFile(String strUnZipPath, String strZipPath) {
        ZipInputStream zipIn = null;
        BufferedOutputStream bufferOut = null;
        boolean bResult = false;

        try {
            strUnZipPath = createSeparator(strUnZipPath);
            zipIn = new ZipInputStream(new BufferedInputStream(new FileInputStream(strZipPath)));

            ZipEntry entry = null;
            byte buffer[] = new byte[4096];
            String strFileName = null;

            while ((entry = zipIn.getNextEntry()) != null) {
                strFileName = entry.getName();

                if (entry.isDirectory()) {
                    File subDir = new File(strUnZipPath + strFileName);
                    subDir.mkdirs();
                    continue;
                }

                bufferOut = new BufferedOutputStream(new FileOutputStream(strUnZipPath));
                int iReadSize = 0;
                while ((iReadSize = zipIn.read(buffer)) > 0) {
                    bufferOut.write(buffer, 0, iReadSize);
                }
                bufferOut.flush();
                bufferOut.close();
                bufferOut = null;
            }
            bResult = true;
        } catch (Exception e) {
        } finally {
            try {
                if (zipIn != null) {
                    zipIn.closeEntry();
                    zipIn.close();
                }

                if (bufferOut != null) {
                    bufferOut.close();
                }
            } catch (Exception e) {
            }
        }
        return bResult;
    }
}
