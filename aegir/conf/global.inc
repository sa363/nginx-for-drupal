<?php # global settings.php

error_reporting(0); // disable reporting errors by default - enable later only for "dev" subdomains

// Fix for Mollom, CloudFlare and others running via Proxy.
if (isset($_SERVER['REMOTE_ADDR'])) {
  if (isset($_SERVER['HTTP_X_FORWARDED_FOR'])) {
    $_SERVER['REMOTE_ADDR'] = $_SERVER['HTTP_X_FORWARDED_FOR'];
  }
  if (isset($_SERVER['HTTP_X_REAL_IP'])) {
    $_SERVER['REMOTE_ADDR'] = $_SERVER['HTTP_X_REAL_IP'];
  }
  if (isset($_SERVER['HTTP_CF_CONNECTING_IP'])) {
    $_SERVER['REMOTE_ADDR'] = $_SERVER['HTTP_CF_CONNECTING_IP'];
  }
}

// Required for proper Redis support on command line / via Drush
if (isset($_SERVER['HTTP_HOST']) && !isset($_SERVER['SERVER_NAME'])) {
  $_SERVER['SERVER_NAME'] = $_SERVER['HTTP_HOST'];
}

$conf['page_compression'] = 0;    // required in Nginx, since it already compresses everything
$conf['boost_crawl_on_cron'] = 0; // deny Boost crawler
$conf['cron_safe_threshold'] = 0; // disable poormanscron
$conf['preprocess_css'] = 1;      // enable hardcoded css aggregation
$conf['preprocess_js'] = 1;       // enable hardcoded js aggregation
$conf['file_downloads'] = 1;      // force public downloads by default
$conf['error_level'] = 0;         // disable errors on screen
$conf['statistics_enable_access_log'] = 0;   // disable access log stats
$conf['allow_authorize_operations'] = FALSE; // disable insecure plugin manager
$conf['admin_menu_cache_client'] = FALSE;    // disable caching in admin_menu #442560
$conf['boost_ignore_htaccess_warning'] = 1;  // silence false alarm in Boost
$conf['expire_flush_front'] = 1;             // default settings for expire module
$conf['expire_flush_node_terms'] = 1;        // default settings for expire module
$conf['expire_flush_menu_items'] = 0;        // default settings for expire module
$conf['expire_flush_cck_references'] = 0;    // default settings for expire module
$conf['expire_include_base_url'] = 1;        // default settings for expire module
$conf['video_ffmpeg_instances'] = 1;         // force safe default for ffmpeg
$conf['securepages_enable'] = 1;             // force to avoid issues with SSL proxy
$conf['drupal_http_request_fails'] = FALSE;  // avoid false alarm
$conf['image_allow_insecure_derivatives'] = TRUE; // enable to avoid known issues: http://drupal.org/drupal-7.20-release-notes

ini_set('session.cookie_lifetime', 86400); // set it to 24 hours max
ini_set('session.cache_expire', 86400);    // set it also to 24 hours max
ini_set('session.gc_maxlifetime', 86400);  // set it also to 24 hours max
// Improve sessions management if module session_expire and/or Pressflow core is not used.
// It is already enabled in the php.ini, but we are forcing it here, just in case.
ini_set('session.gc_probability', 1);
ini_set('session.gc_divisor', 1000);

if (isset($_SERVER['HTTP_USER_AGENT']) && preg_match("/(?:crawl|goog|yahoo|yandex|spider|bot|tracker|click|parser)/i", $_SERVER['HTTP_USER_AGENT'])) {
  // detect known bots
  $known_bot = 'bot';
}

// New Relic - see: https://newrelic.com/docs/php/per-directory-settings#perdir-rollup
if (isset($_SERVER['DOCUMENT_ROOT']) && isset($_SERVER['SERVER_NAME'])) {
  $frags = explode("/", $_SERVER['DOCUMENT_ROOT']);
  if ($frags[1] == 'data') {
    $this_instance = 'Site: ' . $_SERVER['SERVER_NAME'] . ';AAA Octopus ' . $frags[3] . ';AAA All Instances';
  }
  elseif ($frags[2] == 'aegir') {
    $this_instance = 'Site: ' . $_SERVER['SERVER_NAME'] . ';AAA Barracuda Master;AAA All Instances';
  }
  ini_set('newrelic.appname', $this_instance);
  ini_set('newrelic.framework', 'drupal');
  if (extension_loaded('newrelic')) {
    newrelic_set_appname($this_instance);
  }
}

if ($conf['install_profile'] == 'hostmaster' && isset($_SERVER['HTTP_USER_AGENT'])) {
  if (!file_exists('/data/conf/no-https-aegir.inc')) {
    $request_type = ($_SERVER['HTTP_X_FORWARDED_PROTO'] == 'https' || $_SERVER['HTTPS'] == 'on') ? 'SSL' : 'NONSSL';
    if ($request_type != "SSL" && !preg_match("/^\/cron\.php/", $_SERVER['REQUEST_URI'])) { // we force secure connection here
      header('X-Accel-Expires: 5');
      header("HTTP/1.1 301 Moved Permanently");
      header("Location: https://" . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI']);
    }
  }
  $conf['hosting_require_disable_before_delete'] = 0;
  unset($conf['cache']); // disable hardcoded caching
  ini_set('session.cookie_lifetime', 0); // force log-out on browser quit
  if (preg_match("/(?:host8|omega8|aegir\.cc)/i", $_SERVER['HTTP_HOST'])) {
    if (preg_match("/^\/admin\/user\/user\/create/", $_SERVER['REQUEST_URI']) ||
        preg_match("/^\/node\/add\/server/", $_SERVER['REQUEST_URI']) ||
        preg_match("/^\/node\/(?:1|2|4|5|7|8|10)\/(?:edit|delete)/", $_SERVER['REQUEST_URI'])) {
      header('X-Accel-Expires: 5');
      header("HTTP/1.1 301 Moved Permanently");
      header("Location: https://" . $_SERVER['HTTP_HOST'] . "/hosting/sites");
    }
  }
  header('X-Accel-Expires: 1');
}

$da_inc       = FALSE;
$deny_anon    = FALSE;
$drupal_eight = FALSE;
$drupal_five  = FALSE;
$drupal_seven = FALSE;
$drupal_six   = FALSE;
$force_redis  = FALSE;
$high_traffic = FALSE;
$is_cart      = FALSE;
$is_dev       = FALSE;
$redis_lock   = FALSE;
$redis_up     = FALSE;
$use_redis    = FALSE;

/**
 * Optional extra settings
 *
 */
if (file_exists('/data/conf/settings.global.inc')) {
  require_once "/data/conf/settings.global.inc";
}

if (file_exists('./web.config')) {
  if (file_exists('./core')) {
    $conf['cache'] = 1; // enable hardcoded standard caching - Drupal 8
    $drupal_eight = TRUE;
    header('X-Backend: A');
  }
  else {
    $conf['cache'] = 1; // enable hardcoded standard caching - Drupal 7
    $drupal_seven = TRUE;
    $use_redis = TRUE;
    header('X-Backend: B');
  }
}
elseif (file_exists('./modules/path_alias_cache')) {
  $conf['cache'] = 1; // enable hardcoded standard caching - Pressflow 6
  $drupal_six = TRUE;
  $use_redis = TRUE;
  header('X-Backend: C');
}
elseif (file_exists('./modules/watchdog')) {
  $conf['cache'] = 1; // enable hardcoded standard caching - Pressflow 5
  $drupal_five = TRUE;
  header('X-Backend: D');
}
else {
  $conf['cache'] = 1; // enable hardcoded standard caching
  $drupal_unknown = TRUE;
  header('X-Backend: E');
}

// http://drupal.org/node/912682#comment-3503636
if ($conf['install_profile'] == 'managingnews') {
  ini_set('auto_detect_line_endings', TRUE);
}

// JS aggregation breaks jquery.autopager-1.0.0.js
if ($conf['install_profile'] == 'martplug') {
  unset($conf['preprocess_js']);  // disable hardcoded js aggregation
}

// Prevent turning the feature server site into a spam machine
if ($conf['install_profile'] == 'feature_server') {
  $conf['user_register'] = 0; // force "Only site administrators can create new user accounts"
}

if (isset($_SERVER['SERVER_NAME'])) {
  if ($conf['install_profile'] == 'uberdrupal' ||
      $conf['install_profile'] == 'martplug' ||
      $conf['install_profile'] == 'commerce' ||
      $conf['install_profile'] == 'commerce_kickstart' ||
      $conf['install_profile'] == 'opendeals' ||
      file_exists('sites/all/modules/ubercart/README.txt') ||
      file_exists('profiles/' . $conf['install_profile'] . '/modules/ubercart/README.txt') ||
      file_exists('profiles/' . $conf['install_profile'] . '/modules/contrib/ubercart/README.txt') ||
      file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/ubercart/README.txt')) {
    unset($conf['file_downloads']); // disable hardcoded public downloads
    $is_cart = TRUE;
    header('X-Is-Cart: YES');
  }
}

if (isset($_SERVER['HTTP_HOST']) && preg_match("/(?:dev\.|devel\.)/i", $_SERVER['HTTP_HOST']) && isset($_SERVER['HTTP_USER_AGENT'])) {
  // deny known search bots on dev subdomains
  if (!empty($known_bot)) {
    header('X-Accel-Expires: 5');
    header("HTTP/1.1 301 Moved Permanently");
    header("Location: http://www.aegirproject.org/");
  }
  error_reporting(E_ALL & ~E_NOTICE);
  ini_set('display_errors', TRUE);
  ini_set('display_startup_errors', TRUE);
  unset($conf['cache']);          // stop hardcoding caching
  unset($conf['preprocess_css']); // stop hardcoding css aggregation
  unset($conf['preprocess_js']);  // stop hardcoding js aggregation
  unset($conf['error_level']);    // stop hardcoding no errors on screen
  $conf['xmlsitemap_submit'] = 0; // disable XML Sitemap for dev.domain
  $conf['xmlsitemap_update'] = 0; // disable XML Sitemap for dev.domain
  $is_dev = TRUE;
}

if (isset($_SERVER['HTTP_HOST']) && preg_match("/host8/i", $_SERVER['HTTP_HOST'])) {
  // deny known search bots on host8 subdomains
  if (!empty($known_bot)) {
    header('X-Accel-Expires: 5');
    header("HTTP/1.1 301 Moved Permanently");
    header("Location: http://omega8.cc/");
  }
}

$use_cache = TRUE;     // Standard Redis integration for 6.x and 7.x core is enabled by default.

$custom_cache = FALSE; // When set to TRUE in the /data/conf/override.global.inc file,
                       // it will disable default Redis configuration.

$custom_fb = FALSE;    // When set to TRUE in the /data/conf/override.global.inc file,
                       // it will disable default Drupal for Facebook (fb) configuration.

$custom_da = FALSE;    // When set to TRUE in the /data/conf/override.global.inc file,
                       // it will disable default Domain Access configuration,
                       // so you can define your own, custom configuration in the included below
                       // /data/conf/override.global.inc file. NOTE: if set to TRUE, then you
                       // must set to TRUE also $custom_cache and copy all its logic in your
                       // /data/conf/override.global.inc file, because Domain Access must be included
                       // *after* any cache-related settings to work properly.

if (file_exists('/var/run/redis.pid')) {
  $redis_up = TRUE;
}

if (isset($_SERVER['SERVER_NAME']) && (file_exists('sites/all/modules/redis_cache_disable.info') ||
    file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/redis_cache_disable.info'))) {
  $use_cache = FALSE;
}

if (isset($_SERVER['REQUEST_URI']) && preg_match("/noredis=1/", $_SERVER['REQUEST_URI'])) {
  $use_redis = FALSE;
}

if ($redis_up && $use_cache && $use_redis && !$force_redis) {
  header('X-Allow-Redis: YES');
}
elseif ($redis_up && $force_redis) {
  $use_cache = TRUE;
  $use_redis = TRUE;
  header('X-Force-Redis: YES');
}
else {
  if (!$is_dev) {
    $conf['cache'] = 1; // enable standard db caching if redis is not allowed or not available
  }
  header('X-Allow-Redis: NO');
}

if (file_exists('sites/all/modules/domain/settings.inc')) {
  $da_inc = 'sites/all/modules/domain/settings.inc';
}
elseif (file_exists('profiles/' . $conf['install_profile'] . '/modules/domain/settings.inc')) {
  $da_inc = 'profiles/' . $conf['install_profile'] . '/modules/domain/settings.inc';
}
elseif (file_exists('profiles/' . $conf['install_profile'] . '/modules/contrib/domain/settings.inc')) {
  $da_inc = 'profiles/' . $conf['install_profile'] . '/modules/contrib/domain/settings.inc';
}

if (function_exists('drush_get_command')) {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-install/", $command[0])) {
    $use_redis = FALSE;
    $use_cache = FALSE;
    $da_inc    = FALSE;
  }
}

if (isset($_SERVER['HTTP_HOST']) && isset($_SERVER['SERVER_NAME'])) {

  if (isset($_SERVER['HTTP_USER_AGENT']) && isset($_SERVER['USER_DEVICE'])) {
    $this_device = $_SERVER['USER_DEVICE'];
  }
  else {
    $this_device = 'normal';
  }

  if (file_exists('sites/all/modules/advagg/advagg_bundler/advagg_bundler.module') ||
      file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/advagg/advagg_bundler/advagg_bundler.module') ||
      file_exists('profiles/' . $conf['install_profile'] . '/modules/advagg/advagg_bundler/advagg_bundler.module') ||
      file_exists('profiles/' . $conf['install_profile'] . '/modules/contrib/advagg/advagg_bundler/advagg_bundler.module')) {
    $conf['preprocess_css'] = 0; // css aggregation disabled
    $conf['preprocess_js'] = 0;  // js aggregation disabled
    $conf['advagg_aggregate_mode'] = 1;
    $conf['advagg_async_generation'] = 0;
    $conf['advagg_checksum_mode'] = "md5";
    $conf['advagg_closure'] = 1;
    $conf['advagg_css_compress_agg_files'] = 1;
    $conf['advagg_css_compress_compressor_level'] = "sane";
    $conf['advagg_css_compress_inline'] = 1;
    $conf['advagg_css_compressor'] = 2;
    $conf['advagg_debug'] = 0;
    $conf['advagg_dir_htaccess'] = 0;
    $conf['advagg_enabled'] = 1;
    $conf['advagg_gzip_compression'] = 1;
    $conf['advagg_js_compress_agg_files'] = 1;
    $conf['advagg_js_compress_callback'] = 1;
    $conf['advagg_js_compress_inline'] = 1;
    $conf['advagg_js_compress_packer_enable'] = 0;
    $conf['advagg_js_compressor'] = 1;
    $conf['advagg_page_cache_mode'] = 0;
    $conf['advagg_rebuild_on_flush'] = 0;
    $conf['advagg_server_addr'] = "-1";
  }

  if (file_exists('./modules/o_contrib/purge/purge.inc')) {
    $conf['purge_proxy_urls'] = "http://127.0.0.1:8888/purge-normal?purge_method=get http://127.0.0.1:8888/purge-mobile-tablet?purge_method=get http://127.0.0.1:8888/purge-mobile-smart?purge_method=get http://127.0.0.1:8888/purge-mobile-other?purge_method=get";
    header('X-Purge-Level: 6');
  }
  elseif (file_exists('./modules/o_contrib_seven/purge/purge.inc')) {
    $conf['expire_flush_node_terms'] = 0; // disable this to avoid WSOD - see http://drupal.org/node/1151686#comment-5556544
    $conf['purge_proxy_urls'] = "http://127.0.0.1:8888/purge-normal?purge_method=get http://127.0.0.1:8888/purge-mobile-tablet?purge_method=get http://127.0.0.1:8888/purge-mobile-smart?purge_method=get http://127.0.0.1:8888/purge-mobile-other?purge_method=get";
    header('X-Purge-Level: 7');
  }
  else {
    header('X-Purge-Level: none');
  }

  if (isset($_SERVER['HTTP_X_FORWARDED_PROTO']) || isset($_SERVER['HTTPS'])) {
    $conf['https'] = TRUE;
    $request_type = ($_SERVER['HTTP_X_FORWARDED_PROTO'] == 'https' || $_SERVER['HTTPS'] == 'on') ? 'SSL' : 'NONSSL';
    if ($request_type == "SSL") { // we check for secure connection to set correct base_url
      $base_url = 'https://' . $_SERVER['HTTP_HOST'];
      if ($conf['install_profile'] != 'hostmaster') {
        $_SERVER['HTTPS'] = 'on';
        ini_set('session.cookie_secure', TRUE);
        header('X-Cookie-Sec: YES');
      }
      header('X-Local-Proto: https');
    }
    else {
      $base_url = 'http://' . $_SERVER['HTTP_HOST'];
    }
  }
  else {
    $base_url = 'http://' . $_SERVER['HTTP_HOST'];
  }

  if (file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/cookie_domain.info')) {
    $domain = '.'. preg_replace('`^www.`', '', $_SERVER['SERVER_NAME']);
  }
  else {
    $domain = '.'. preg_replace('`^www.`', '', $_SERVER['HTTP_HOST']);
  }
  $domain = str_replace('..', '.', $domain);
  if (count(explode('.', $domain)) > 2 && !is_numeric(str_replace('.', '', $domain))) {
    @ini_set('session.cookie_domain', $domain);
    $cookie_domain = $domain;
    header("X-Cookie-Domain: " . $cookie_domain);
  }

  $this_prefix = $_SERVER['SERVER_NAME'] . '_';
  header("X-Redis-Prefix: " . $this_prefix);

  if (isset($_SERVER['REQUEST_TIME']) && isset($_SERVER['REMOTE_ADDR']) && isset($_SERVER['HTTP_USER_AGENT']) &&
      !preg_match("/^\/esi\//", $_SERVER['REQUEST_URI'])) {
    $identity = $_SERVER['REQUEST_TIME'] . $_SERVER['REMOTE_ADDR'] . $_SERVER['SERVER_NAME'] . $_SERVER['HTTP_USER_AGENT'];
    $identity = 'BOND'. md5("$identity");

    if ($drupal_seven || $drupal_eight) {
      $conf['https'] = TRUE;
      $sess_prefix = ini_get('session.cookie_secure') ? 'SSESS' : 'SESS';
      $test_sess_name = $sess_prefix . substr(hash('sha256', $cookie_domain), 0, 32);
      unset($conf['file_downloads']); // disable hardcoded public downloads in d7
    }
    else {
      $test_sess_name = 'SESS'. md5($cookie_domain);
    }

    if (!isset($_COOKIE[$test_sess_name])) {
      if (file_exists('sites/all/modules/nginx_cache_quarter.info') ||
          file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/nginx_cache_quarter.info')) {
        $expire_in_seconds = '900';
        header('X-Limit-Booster: 900');
      }
      elseif (file_exists('sites/all/modules/nginx_cache_hour.info') || $force_redis || $high_traffic ||
          file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/nginx_cache_hour.info')) {
        $expire_in_seconds = '3600';
        header('X-Limit-Booster: 3600');
      }
      elseif (file_exists('sites/all/modules/nginx_cache_day.info') ||
          file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/nginx_cache_day.info')) {
        $expire_in_seconds = '86400';
        header('X-Limit-Booster: 86400');
      }
    }

    if (!isset($_COOKIE[$test_sess_name])) {
      if (preg_match("/\/(?:node\/[0-9]+\/edit|node\/add)/", $_SERVER['REQUEST_URI'])) {
        if (!file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/allow_anon_node_add.info')) {
          $deny_anon = TRUE;
          header("HTTP/1.1 301 Moved Permanently");
          header("Location: " . $base_url . "/");
        }
      }
      if (preg_match("/^\/(?:[a-z]{2}\/)?(?:admin|logout|privatemsg|approve)/", $_SERVER['REQUEST_URI'])) {
        if (!file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/disable_admin_dos_protection.info')) {
          $deny_anon = TRUE;
          header("HTTP/1.1 301 Moved Permanently");
          header("Location: " . $base_url . "/");
        }
      }
    }

    if (empty($known_bot) && !$high_traffic && !$deny_anon) {
      if (preg_match("/^\/(?:[a-z]{2}\/)?(?:user|admin|cart|checkout|logout|privatemsg)/", $_SERVER['REQUEST_URI']) ||
          preg_match("/\/(?:node\/[0-9]+\/edit|node\/add|comment\/reply|approve|users|ajax_comments|commerce_currency_select)/", $_SERVER['REQUEST_URI']) ||
          preg_match("/(?:dev\.|devel\.)/", $_SERVER['HTTP_HOST'])) {
        if (!isset($_COOKIE['OctopusNoCacheID'])) {
          $lifetime = '15';
          setcookie('OctopusNoCacheID', 'NOCACHE' . $identity, $_SERVER['REQUEST_TIME'] + $lifetime, '/', $cookie_domain);
        }
        $expire_in_seconds = '1';
        header('X-Limit-Booster: 1');
      }

      if (isset($_SERVER['REQUEST_URI']) && preg_match("/(?:x-progress-id|ahah|progress\/|autocomplete|ajax|batch|js\/.*)/i", $_SERVER['REQUEST_URI'])) {
        $expire_in_seconds = '0';
        header('X-Skip-Booster: AjaxRU');
      }

      if (isset($_SERVER['QUERY_STRING']) && preg_match("/(?:x-progress-id|ahah|progress\/|autocomplete|ajax|batch|js\/.*)/i", $_SERVER['QUERY_STRING'])) {
        $expire_in_seconds = '0';
        header('X-Skip-Booster: AjaxQS');
      }

      if (isset($_SERVER['REQUEST_METHOD']) && $_SERVER['REQUEST_METHOD'] == 'POST') {
        if (!isset($_COOKIE['OctopusNoCacheID'])) {
          $lifetime = '15';
          setcookie('OctopusNoCacheID', 'POST' . $identity, $_SERVER['REQUEST_TIME'] + $lifetime, '/', $cookie_domain);
        }
        $expire_in_seconds = '0';
        header('X-Skip-Booster: PostRM');
      }
    }

    if (!empty($known_bot)) {
      if (!preg_match("/Pingdom/i", $_SERVER['HTTP_USER_AGENT'])) {
        $expire_in_seconds = '86400';
        header('X-Bot-Booster: 86400');
      }
    }

    if ($conf['install_profile'] != 'hostmaster' && (!empty($expire_in_seconds) || $expire_in_seconds == 0)) {
      header("X-Accel-Expires: " . $expire_in_seconds);
    }
  }
}

/**
 * Optional extra overrides
 *
 */
if (file_exists('/data/conf/override.global.inc')) {
  require_once "/data/conf/override.global.inc";
}

// Use Redis caching and lock support only for d6 and d7 profiles.
if ($redis_up && $use_cache && $use_redis && !$custom_cache) {
  $cache_backport = FALSE;
  $cache_redis = FALSE;
  if (file_exists('./modules/o_contrib/cache_backport/cache.inc')) {
    $cache_backport = TRUE;
    $cache_backport_path = './modules/o_contrib/cache_backport/cache.inc';
  }
  if (file_exists('./modules/o_contrib_seven/redis/redis.autoload.inc')) {
    $cache_redis = TRUE;
    $cache_backport = FALSE;
    $cache_redis_path = './modules/o_contrib_seven/redis/redis.autoload.inc';
    $cache_lock_path = './modules/o_contrib_seven/redis/redis.lock.inc';
  }
  elseif (file_exists('./modules/o_contrib/redis/redis.autoload.inc')) {
    $cache_redis = TRUE;
    $cache_redis_path = './modules/o_contrib/redis/redis.autoload.inc';
    $cache_lock_path = './modules/o_contrib/redis/redis.lock.d6.inc';
  }
  if ($cache_redis) {
    if ($cache_backport) {
      $conf['cache_inc']                      = $cache_backport_path;
    }
    if (file_exists('sites/all/modules/redis_lock_enable.info') ||
        file_exists('sites/'. $_SERVER['SERVER_NAME'] .'/modules/redis_lock_enable.info')) {
      $redis_lock = TRUE;
    }
    if (file_exists($cache_lock_path) && $redis_lock) {
      $conf['lock_inc']                       = $cache_lock_path;
    }
    $conf['cache_backends'][]                 = $cache_redis_path;
    $conf['redis_client_interface']           = 'PhpRedis';
    $conf['redis_client_host']                = '127.0.0.1';
    $conf['redis_client_port']                = '6379';
    $conf['redis_client_password']            = 'isfoobared';
    $conf['cache_prefix']                     = $this_prefix;
    $conf['page_cache_invoke_hooks']          = TRUE;
    $conf['page_cache_without_database']      = FALSE;
    $conf['page_cache_maximum_age']           = 0;
    $conf['page_cache_max_age']               = 0;
    $conf['cache_lifetime']                   = 0;
    $conf['cache_default_class']              = 'Redis_Cache';
    $conf['cache_class_cache_form']           = 'DrupalDatabaseCache';
    if ($conf['install_profile'] == 'hostmaster') {
      $conf['cache_class_cache']              = 'DrupalDatabaseCache';
    }
  }
}


/**
 * Drupal for Facebook (fb)
 *
 * Important:
 * Facebook client libraries will not work properly if arg_separator.output is not &.
 * The default value is &amp;. Change this in settings.php. Make the value "&"
 * http://drupal.org/node/205476
 */
if (!$custom_fb) {
  if (file_exists('sites/all/modules/fb/fb_settings.inc')) {
    ini_set('arg_separator.output', '&');
    require_once "sites/all/modules/fb/fb_settings.inc";
    $conf['fb_api_file'] = "sites/all/modules/fb/facebook-platform/php/facebook.php";
  }
  elseif (file_exists('profiles/' . $conf['install_profile'] . '/modules/fb/fb_settings.inc')) {
    ini_set('arg_separator.output', '&');
    require_once 'profiles/' . $conf['install_profile'] . '/modules/fb/fb_settings.inc';
    $conf['fb_api_file'] = 'profiles/' . $conf['install_profile'] . '/modules/fb/facebook-platform/php/facebook.php';
  }
  elseif (file_exists('profiles/' . $conf['install_profile'] . '/modules/contrib/fb/fb_settings.inc')) {
    ini_set('arg_separator.output', '&');
    require_once 'profiles/' . $conf['install_profile'] . '/modules/contrib/fb/fb_settings.inc';
    $conf['fb_api_file'] = 'profiles/' . $conf['install_profile'] . '/modules/contrib/fb/facebook-platform/php/facebook.php';
  }
}

/**
 * Domain module
 *
 */
if (!$custom_da) {
  if ($da_inc) {
    require_once($da_inc);
  }
}
