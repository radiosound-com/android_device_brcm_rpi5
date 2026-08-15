/*
 * Copyright 2026 Radio Sound, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.radiosound.caramelvoicedefaults;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.app.AppOpsManager;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;

/** Selects the product's offline TTS engine when the active user has no choice yet. */
public final class VoiceDefaultsReceiver extends BroadcastReceiver {
    private static final String TAG = "CaramelVoiceDefaults";
    private static final String TTS_DEFAULT_SYNTH = "tts_default_synth";
    private static final String ESPEAK_ENGINE = "com.reecedunn.espeak";
    private static final String KOKORO_ENGINE = "com.k2fsa.sherpa.onnx.tts.engine";
    private static final String TTS_CONFIG = "/product/etc/caramel_voice/tts.properties";
    private static final String CONTROL_AUDIO = "android:control_audio";
    private static final String CONTROL_AUDIO_PARTIAL = "android:control_audio_partial";

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        if (!Intent.ACTION_LOCKED_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_USER_UNLOCKED.equals(action)) {
            return;
        }

        final ContentResolver resolver = context.getContentResolver();
        final String current = Settings.Secure.getString(resolver, TTS_DEFAULT_SYNTH);
        if (current == null || current.isEmpty()) {
            final String configured = configuredEngine();
            final String engine = isInstalled(context, configured) ? configured : ESPEAK_ENGINE;
            if (!isInstalled(context, engine)
                    || !Settings.Secure.putString(resolver, TTS_DEFAULT_SYNTH, engine)) {
                Log.e(TAG, "Unable to select the offline eSpeak TTS engine");
                return;
            }
        }
        allowBundledTtsPlaybackForActiveUser(context);
        Log.i(TAG, "Preserved or selected the offline TTS engine and playback policy for the active user");
    }

    /**
     * Audio hardening defaults CONTROL_AUDIO app-ops to foreground-only. The bundled TTS engine
     * is a background bound service while it synthesizes, so allow only this product-owned engine
     * to play speech. This is intentionally package-scoped and does not disable hardening globally.
     */
    private static void allowBundledTtsPlaybackForActiveUser(Context context) {
        allowTtsPlaybackForEngine(context, ESPEAK_ENGINE);
        allowTtsPlaybackForEngine(context, KOKORO_ENGINE);
    }

    private static void allowTtsPlaybackForEngine(Context context, String engine) {
        final int userId = UserHandle.myUserId();
        final int uid;
        try {
            uid = context.getPackageManager().getPackageUid(engine, 0);
        } catch (PackageManager.NameNotFoundException exception) {
            Log.i(TAG, "Bundled TTS engine is not installed for user " + userId + ": " + engine);
            return;
        }

        final AppOpsManager appOps = context.getSystemService(AppOpsManager.class);
        appOps.setMode(CONTROL_AUDIO, uid, engine, AppOpsManager.MODE_ALLOWED);
        appOps.setMode(CONTROL_AUDIO_PARTIAL, uid, engine, AppOpsManager.MODE_ALLOWED);
        Log.i(TAG, "Allowed offline TTS playback hardening app-ops for " + engine
                + " uid " + uid + " user " + userId);
    }

    private static boolean isInstalled(Context context, String engine) {
        if (engine == null || engine.isEmpty()) return false;
        try {
            context.getPackageManager().getPackageInfo(engine, 0);
            return true;
        } catch (PackageManager.NameNotFoundException exception) {
            return false;
        }
    }

    private static String configuredEngine() {
        Properties properties = new Properties();
        try (FileInputStream input = new FileInputStream(TTS_CONFIG)) {
            properties.load(input);
            String engine = properties.getProperty("engine");
            return engine == null || engine.isEmpty() ? ESPEAK_ENGINE : engine.trim();
        } catch (IOException exception) {
            Log.i(TAG, "No product TTS configuration; using eSpeak fallback", exception);
            return ESPEAK_ENGINE;
        }
    }
}
