function close()
    error_code = cepton_sdk.c.call('cepton_sdk_capture_replay_close');
    cepton_sdk.c.check_error_code(error_code);
    cepton_sdk.internal.clear_cache()
end
