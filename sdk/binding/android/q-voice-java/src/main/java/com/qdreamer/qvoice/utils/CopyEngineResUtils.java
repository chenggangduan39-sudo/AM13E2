package com.qdreamer.qvoice.utils;

import android.content.Context;
import android.content.res.AssetManager;
import android.text.TextUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class CopyEngineResUtils {

    /**
     * 将 assets 下的所有文件及文件夹拷贝到本地 dir 目录下
     */
    public static void copyAssets2Local(Context context, File dir) throws Exception {
        copyAssets2Local(context, "", dir);
    }

    /**
     * 将 assets 下名为 [assetsPath] 的文件或文件夹拷贝到本地 [dir] 目录中
     */
    public static void copyAssets2Local(Context context, String assetsPath, File dir) throws Exception {
        copyAssets2Local(context, getAllAssetsFile(context.getAssets(), assetsPath), dir);
    }

    /**
     * 用于对比 assets 目录下的文件 MD5 值，确认本地拷贝过的文件是否被修改而需要重新拷贝；
     * <p>
     * 好处是：不需要每次对 [assetsPath] 目录下的全部文件重新拷贝，而是替换其中 MD5 值不匹配的文件
     * <p>
     * 坏处是：每次资源文件更新，[assetsMD5Map] 中的配置就需要同步增删改
     *
     * @param assetsMD5Map key 是 assets 目录下的文件全路径名；value 是该文件对应的 md5 值，不设置表示不校验值，只确保文件存在
     */
    public static void copyAssets2Local(Context context, Map<String, String> assetsMD5Map, File dir) throws Exception {
        if (assetsMD5Map.isEmpty()) {
            return;
        }
        // 记录需要拷贝到本地的文件，即本地不存在或 md5 校验失败的
        Set<String> needCopyAssetFileSet = new HashSet<>();
        for (Map.Entry<String, String> entry : assetsMD5Map.entrySet()) {
            String filePath = entry.getKey();
            String md5Value = entry.getValue();
            File resFile = new File(dir, filePath);
            if (resFile.exists() && resFile.isFile()) {
                if (!TextUtils.isEmpty(md5Value) && !md5Value.equals(getFileMD5(resFile))) {
                    needCopyAssetFileSet.add(filePath);
                }
            } else {
                needCopyAssetFileSet.add(filePath);
            }
        }
        if (!needCopyAssetFileSet.isEmpty()) {
            AssetManager assetManager = context.getAssets();
            for (String filePath : needCopyAssetFileSet) {
                File localFile = new File(dir, filePath);
                if (!localFile.getParentFile().exists() || !localFile.getParentFile().isDirectory()) {
                    localFile.getParentFile().mkdirs();
                }
                InputStream inputStream = assetManager.open(filePath);
                OutputStream outputStream = new FileOutputStream(localFile);
                byte[] buffer = new byte[4096];
                int len;
                while ((len = inputStream.read(buffer)) != -1) {
                    outputStream.write(buffer, 0, len);
                }
                outputStream.close();
                inputStream.close();
            }
        }
    }

    private static Map<String, String> getAllAssetsFile(AssetManager assetManager, String assetsPath) throws IOException {
        /*
         注意此处，可以用来判断文件还是文件夹，但是，传入的 assetsPath 不能是空文件夹，
         因为返回的数组长度为 0，和文件一样，不能将其当做文件处理 open()
         但是，如果是 assetsPath 下面有文件也有空文件夹，空文件夹不会列出来
         */
        String[] files = assetManager.list(assetsPath);
        Map<String, String> map = new HashMap<>();
        if (files == null || files.length == 0) {   // 文件
            if (!TextUtils.isEmpty(assetsPath)) {
                map.put(assetsPath, "?"); // 随便标记一个 md5 值，后续校验必然不通过，重新拷贝资源
            }
        } else {    // 文件夹
            for (String file : files) {
                map.putAll(getAllAssetsFile(assetManager, TextUtils.isEmpty(assetsPath) ? file : assetsPath + "/" + file));
            }
        }
        return map;
    }

    private static String getFileMD5(File file) {
        if (!file.exists() || !file.isFile()) {
            return "";
        }
        InputStream inputStream = null;
        byte[] buffer = new byte[1024];
        int len;
        try {
            inputStream = new FileInputStream(file);
            MessageDigest md5 = MessageDigest.getInstance("MD5");
            while ((len = inputStream.read(buffer)) != -1) {
                md5.update(buffer, 0, len);
            }
            BigInteger bi = new BigInteger(1, md5.digest());
            String value = bi.toString(16);
            if (value.length() < 32) {
                int count = 32 - value.length();
                value = String.format("%0" + count + "d", 0) + value;
            }
            return value.toUpperCase();
        } catch (Exception e) {
            e.printStackTrace();
            return "";
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

}
