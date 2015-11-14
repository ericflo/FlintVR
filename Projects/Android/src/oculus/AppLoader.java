package oculus;

import android.util.Base64;
import android.util.Log;

import com.squareup.moshi.JsonAdapter;
import com.squareup.moshi.Moshi;
import com.squareup.okhttp.HttpUrl;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import okio.BufferedSink;
import okio.Okio;

public class AppLoader {
  OkHttpClient mClient = null;
  Moshi mMoshi = null;
  JsonAdapter<AppLoadManifest> mAdapter;

  AppLoadState mState = AppLoadState.IDLE;
  File mCacheDir = null;
  String mUri = null;
  ArrayList<String> mLoaded;

  // Downloaded
  AppLoadManifest mManifest = null;

  // Error state
  Exception mExc = null;

  public AppLoader(File cacheDir, String uri) {
    mClient = new OkHttpClient();
    mMoshi = new Moshi.Builder().build();
    mAdapter = mMoshi.adapter(AppLoadManifest.class);
    mState = AppLoadState.IDLE;
    mCacheDir = cacheDir;
    mUri = uri;
    mLoaded = new ArrayList<String>();
  }

  public static File BaseDir(File cacheDir, String uri) {
    final String pathSegment = Base64.encodeToString(uri.getBytes(), Base64.DEFAULT);
    return new File(cacheDir, pathSegment);
  }

  private String manifestUri() {
    final HttpUrl url = HttpUrl.parse(mUri);
    final List<String> segments = url.pathSegments();
    if (segments.size() > 0 && segments.get(segments.size() - 1).equals("flint.json")) {
      return mUri;
    }
    return url.resolve("flint.json").toString();
  }

  private String fileUri(String path) {
    return HttpUrl.parse(mUri).resolve(path).toString();
  }

  public String filePath(String path) {
    final String baseDir;
    try {
      baseDir = BaseDir(mCacheDir, mUri).getCanonicalPath().trim();
    } catch (IOException e) {
      Log.e("Flint", "Could not get base dir: " + e.getLocalizedMessage());
      return "";
    }
    // TODO: The following probably needs error checking and could be a security risk
    return (baseDir + "/" + path).replace("//", "/");
  }

  private boolean fetchManifest() {
    Log.i("Flint", "Fetching manifest");

    // Check against our current state
    if (mState != AppLoadState.IDLE) {
      mExc = new IllegalStateException("Cannot reuse a loader");
      mState = AppLoadState.ERROR;
      return false;
    }

    // Update our state
    mState = AppLoadState.FETCHING_MANIFEST;

    // Build our request
    final Request request = new Request.Builder().url(manifestUri()).build();

    // Load our response
    try {
      final Response response = mClient.newCall(request).execute();
      mManifest = mAdapter.fromJson(response.body().source());
    } catch (IOException e) {
      // On error, note the error and return failure
      mExc = e;
      mState = AppLoadState.ERROR;
      return false;
    }

    Log.i("Flint", "Manifest fetched");

    // Return success
    return true;
  }

  private boolean loadNextFile() {
    Log.i("Flint", "Loading file");

    // Get the current file path
    final String path = mManifest.files.get(mLoaded.size());
    Log.i("Flint", "Loading file at URI: " + fileUri(path));

    // Create a file object and potentially create the parent directory
    final File downloadedFile = new File(filePath(path));
    final File parentDir = downloadedFile.getParentFile();
    if (!parentDir.exists() && !parentDir.mkdirs()) { // Make sure the dir exists
      mExc = new IOException("Could not load file " + path);
      mState = AppLoadState.ERROR;
      return false;
    }

    // Define the sink where we'll stream our file object
    final BufferedSink sink;
    try {
      sink = Okio.buffer(Okio.sink(downloadedFile));
    } catch (FileNotFoundException e) {
      mExc = e;
      mState = AppLoadState.ERROR;
      return false;
    }

    // Make the request and stream the data to the file, finally closing it
    final Request request = new Request.Builder().url(fileUri(path)).build();
    final Response response;
    try {
      response = mClient.newCall(request).execute();
      sink.writeAll(response.body().source());
      sink.close();
    } catch (IOException e) {
      e.printStackTrace();
    }

    mLoaded.add(path);

    Log.i("Flint", "File loaded at URI: " + fileUri(path));

    return true;
  }

  private boolean loadFiles() {
    while (mLoaded.size() != mManifest.files.size() && mState != AppLoadState.ERROR) {
      if (!loadNextFile()) {
        return false;
      }
    }
    return true;
  }

  public boolean Load() {
    if (!fetchManifest()) {
      return false;
    }
    if (!loadFiles()) {
      return false;
    }
    if (mState == AppLoadState.ERROR) {
      return false;
    }
    mState = AppLoadState.COMPLETE;
    return true;
  }
}
