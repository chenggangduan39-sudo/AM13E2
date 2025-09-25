package com.qdreamer.qvoice.utils

import android.content.Context
import android.content.res.AssetManager
import java.io.*
import java.math.BigInteger
import java.security.MessageDigest
import java.util.*

object CopyEngineResUtils {

    /**
     * 将 assets 下的所有文件及文件夹拷贝到本地 [dir] 目录下
     */
    @Throws(Exception::class)
    fun copyAssets2Local(context: Context, dir: File) {
        copyAssets2Local(context, "", dir)
    }

    /**
     * 将 assets 下名为 [assetsPath] 的文件或文件夹拷贝到本地 [dir] 目录中
     */
    @Throws(Exception::class)
    fun copyAssets2Local(context: Context, assetsPath: String, dir: File) {
        copyAssets2Local(context, getAllAssetsFile(context.assets, assetsPath), dir)
    }

    /**
     * 用于对比 assets 目录下的文件 MD5 值，确认本地拷贝过的文件是否被修改而需要重新拷贝；
     *
     * 好处是：不需要每次对 [assetsPath] 目录下的全部文件重新拷贝，而是替换其中 MD5 值不匹配的文件
     *
     * 坏处是：每次资源文件更新，[assetsMD5Map] 中的配置就需要同步增删改
     *
     * [assetsMD5Map] key 是 assets 目录下的文件全路径名；value 是该文件对应的 md5 值，不设置表示不校验值，只确保文件存在
     */
    @Throws(Exception::class)
    fun copyAssets2Local(context: Context, assetsMD5Map: Map<String, String>, dir: File) {
        if (assetsMD5Map.isEmpty()) {
            return
        }
        // 记录需要拷贝到本地的文件，即本地不存在或 md5 校验失败的
        val needCopyAssetFileSet = HashSet<String>()
        for ((filePath, md5Value) in assetsMD5Map) {
            val resFile = File(dir, filePath)
            if (resFile.exists() && resFile.isFile) {
                if (md5Value.isNotEmpty() && md5Value != getFileMD5(resFile)) {
                    needCopyAssetFileSet.add(filePath)
                }
            } else {
                needCopyAssetFileSet.add(filePath)
            }
        }
        if (needCopyAssetFileSet.isNotEmpty()) {
            val assetManager = context.assets
            needCopyAssetFileSet.forEach { filePath ->
                val localFile = File(dir, filePath)
                if (!localFile.parentFile!!.exists() || !localFile.parentFile!!.isDirectory) {
                    localFile.parentFile!!.mkdirs()
                }
                val inputStream = assetManager.open(filePath)
                localFile.writeBytes(byteArrayOf())
                val buffer = ByteArray(4096)
                var len: Int
                // Failed to allocate a 181165421 byte allocation with 6291456 free bytes and 121MB until OOM
                // inputStream.readBytes() 文件过大时，一次读取所有将会导致内存溢出
                while (inputStream.read(buffer, 0, buffer.size).also { len = it } != -1) {
                    if (len == buffer.size) {
                        localFile.appendBytes(buffer)
                    } else if (len > 0) {
                        localFile.appendBytes(buffer.copyOfRange(0, len))
                    }
                }
            }
        }
    }

    private fun getAllAssetsFile(assetManager: AssetManager, assetsPath: String): Map<String, String> {
        /*
         注意此处，可以用来判断文件还是文件夹，但是，传入的 assetsPath 不能是空文件夹，
         因为返回的数组长度为 0，和文件一样，不能将其当做文件处理 open()
         但是，如果是 assetsPath 下面有文件也有空文件夹，空文件夹不会列出来
         */
        val files = assetManager.list(assetsPath)
        if (files.isNullOrEmpty()) { // 文件
            if (assetsPath.isNotEmpty()) {
                return mapOf(assetsPath to "?") // 随便标记一个 md5 值，后续校验必然不通过，重新拷贝资源
            }
            return emptyMap()
        } else {    // 文件夹
            val map = HashMap<String, String>()
            files.forEach { file ->
                val sonMap = getAllAssetsFile(assetManager, if (assetsPath.isNotEmpty()) "$assetsPath/$file" else file)
                map.putAll(sonMap)
            }
            return map
        }
    }

    private fun getFileMD5(file: File): String {
        if (!file.exists() || !file.isFile) {
            return ""
        }
        val inputStream: FileInputStream
        val buffer = ByteArray(1024)
        var len: Int
        return try {
            inputStream = FileInputStream(file)
            val md5 = MessageDigest.getInstance("MD5")
            while (inputStream.read(buffer, 0, buffer.size).also { len = it } != -1) {
                md5.update(buffer, 0, len)
            }
            val bi = BigInteger(1, md5.digest())
            var value = bi.toString(16)
            if (value.length < 32) {
                val count = 32 - value.length
                value = String.format("%0${count}d", 0) + value
            }
            inputStream.close()
            value.toUpperCase(Locale.getDefault())
        } catch (e: Exception) {
            e.printStackTrace()
            ""
        }
    }

}