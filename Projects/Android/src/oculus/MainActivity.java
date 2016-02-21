package oculus;

import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import com.oculus.vrappframework.VrActivity;

import java.io.File;
import java.io.IOException;

public class MainActivity extends VrActivity {
  public static final String TAG = "Flint";

  private AppLoader mLoader;

  static {
    Log.d(TAG, "LoadLibrary");
    System.loadLibrary("ovrapp");
  }

  public static native long nativeSetAppInterface(VrActivity act, String fromPackageNameString, String commandString, String uriString, AssetManager mgr);

  public boolean loadApp(String uri) {
    mLoader = new AppLoader(getCacheDir(), uri);
    if (!mLoader.Load()) {
      Log.e("Flint", mLoader.mExc.getLocalizedMessage());
      return false;
    }
    return true;
  }

  public String getAppTitle() {
    return mLoader.mManifest.title;
  }

  public String getAppEntrypoint() {
    return mLoader.filePath(mLoader.mManifest.entrypoint);
  }

  public String getBaseDir() {
    try {
      return AppLoader.BaseDir(getCacheDir(), mLoader.mUri).getCanonicalPath().trim();
    } catch (IOException e) {
      Log.e("Flint", "Could not get base dir: " + e.getLocalizedMessage());
      return "";
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    Intent intent = getIntent();
    String commandString = VrActivity.getCommandStringFromIntent(intent);
    String fromPackageNameString = VrActivity.getPackageStringFromIntent(intent);
    String uriString = VrActivity.getUriStringFromIntent(intent);
    AssetManager mgr = getResources().getAssets();

    setAppPtr(nativeSetAppInterface(this, fromPackageNameString, commandString, uriString, mgr));
  }   
}
