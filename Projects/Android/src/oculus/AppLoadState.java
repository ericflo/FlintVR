package oculus;

public enum AppLoadState {
    IDLE,
    FETCHING_MANIFEST,
    DOWNLOADING_FILES,
    COMPLETE,
    ERROR
}