package com.qdreamer.example.utils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

/**
 * @Author: Pen
 * @Create: 2023-02-02 09:26:10
 * @Signature: 佛祖保佑 ——> 没有 Bug.
 */
public class FileUtils {

    /**
     * @param file 判断文件是否存在
     */
    public static boolean isExistFile(File file) {
        return file != null && file.exists() && file.isFile();
    }

    /**
     * @param dir 判断文件夹是否存在
     */
    public static boolean isExistDir(File dir) {
        return dir != null && dir.exists() && dir.isDirectory();
    }

    public static boolean mkdirs(File dir) {
        if (!isExistDir(dir)) {
            return dir.mkdirs();
        }
        return true;
    }

    /**
     * 以数据流形式读取文件
     */
    public static byte[] readBytes(File file) {
        if (!isExistFile(file)) {
            return null;
        }
        InputStream inputStream = null;
        try {
            inputStream = new FileInputStream(file);
            long total = file.length();
            if (total <= 0) {
                return new byte[0];
            } else if (total > Integer.MAX_VALUE) {
                throw new OutOfMemoryError("File " + file.getAbsolutePath() + " is too big (" + total + " bytes) to fit in memory.");
            }
            byte[] totalBuffer = new byte[(int) total];
            byte[] buffer = new byte[4096];
            int len, offset = 0;
            while ((len = inputStream.read(buffer)) != -1) {
                System.arraycopy(buffer, 0, totalBuffer, offset, len);
                offset += len;
            }
            return totalBuffer;
        } catch (Exception e) {
            return null;
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException ignore) {}
            }
        }
    }

    /**
     * 往文件中写入流数据（覆盖）
     */
    public static void writeBytes(File file, byte[] array) {
        bytes2File(file, array, false);
    }

    /**
     * 往文件中写入流数据（追加）
     */
    public static void appendBytes(File file, byte[] array) {
        bytes2File(file, array, true);
    }

    /**
     * 以文本形式读取文件
     */
    public static String readText(File file) {
        if (!isExistFile(file)) {
            return null;
        }
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            StringBuilder builder = new StringBuilder();
            String line;
            while (null != (line = reader.readLine())) {
                builder.append(line);
            }
            return builder.toString();
        } catch (Exception e) {
            return null;
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ignore) {}
            }
        }
    }

    /**
     * 往文件中写入文本数据（覆盖）
     */
    public static void writeText(File file, String text) {
        bytes2File(file, text.getBytes(StandardCharsets.UTF_8), false);
    }

    /**
     * 往文件中写入文本数据（追加）
     */
    public static void appendText(File file, String text) {
        bytes2File(file, text.getBytes(StandardCharsets.UTF_8), true);
    }

    private static void bytes2File(File file, byte[] array, boolean append) {
        mkdirs(file.getParentFile());
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(file, append);
            outputStream.write(array);
            outputStream.flush();
        } catch (Exception ignore) {
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException ignore) {}
            }
        }
    }

    /**
     * 删除文件和文件夹，对应文件夹的目录也会被删除
     */
    public static void deleteFile(File file) {
        deleteFile(file, true);
    }

    /**
     *
     * @param file              需要删除的文件夹目录下的所有子文件
     * @param needDeleteDir     是否将文件夹也进行删除, true 对应文件夹也会删除，false 则只删除文件夹中的文件
     */
    public static void deleteFile(File file, boolean needDeleteDir) {
        if (file == null || !file.exists()) {
            return;
        }
        if (file.isFile()) {
            file.delete();
        } else if (file.isDirectory()) {
            File[] files = file.listFiles();
            if (files != null && files.length > 0) {
                for (File sonFile : files) {
                    if (sonFile.isFile()) {
                        sonFile.delete();
                    } else {
                        deleteFile(sonFile, needDeleteDir);
                    }
                }
            }
            if (needDeleteDir) {
                file.delete();
            }
        }
    }

}
