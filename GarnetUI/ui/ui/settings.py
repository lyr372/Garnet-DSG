"""
Django settings for ui project.

Generated by 'django-admin startproject' using Django 4.2.6.

For more information on this file, see
https://docs.djangoproject.com/en/4.2/topics/settings/

For the full list of settings and their values, see
https://docs.djangoproject.com/en/4.2/ref/settings/
"""

from pathlib import Path

import pytz

# Build paths inside the project like this: BASE_DIR / 'subdir'.
BASE_DIR = Path(__file__).resolve().parent.parent


# region 需要自定义设置的内容
# garnet文件夹
GARNETPATH = "GARNETPATH"

# Metadata
NAME = "NAME"
IPADDRESS = "IPADDRESS"
PORT = 8000

# SECURITY WARNING: keep the secret key used in production secret!
SECRET_KEY = "django-insecure-3pn^b24iqv(2*ta^rq+%)ghl2--z7*qk4y7mkznai16_jx&@c8"
# endregion

# region 开发时启用
# SECURITY WARNING: don't run with debug turned on in production!
DEBUG = True

ALLOWED_HOSTS = ["*"]
# 允许全部来源，开发时设置
CORS_ORIGIN_ALLOW_ALL = True
CORS_ALLOW_ALL_ORIGINS = True
CORS_ALLOW_CREDENTIALS = True
CORS_ALLOW_HEADERS = "*"
# endregion
# Application definition

INSTALLED_APPS = [
    "django.contrib.admin",
    "django.contrib.auth",
    "django.contrib.contenttypes",
    "django.contrib.sessions",
    "django.contrib.messages",
    "django.contrib.staticfiles",
    "rest_framework",
    "Model",
    "drf_spectacular",
    "django_q",
    "corsheaders",
]

MIDDLEWARE = [
    "django.middleware.security.SecurityMiddleware",
    "django.contrib.sessions.middleware.SessionMiddleware",
    "corsheaders.middleware.CorsMiddleware",
    "django.middleware.common.CommonMiddleware",
    "django.middleware.csrf.CsrfViewMiddleware",
    "django.contrib.auth.middleware.AuthenticationMiddleware",
    "django.contrib.messages.middleware.MessageMiddleware",
    "django.middleware.clickjacking.XFrameOptionsMiddleware",
]

ROOT_URLCONF = "ui.urls"

TEMPLATES = [
    {
        "BACKEND": "django.template.backends.django.DjangoTemplates",
        "DIRS": [Path.joinpath(BASE_DIR, "templates")],
        "APP_DIRS": True,
        "OPTIONS": {
            "context_processors": [
                "django.template.context_processors.debug",
                "django.template.context_processors.request",
                "django.contrib.auth.context_processors.auth",
                "django.contrib.messages.context_processors.messages",
            ],
        },
    },
]

WSGI_APPLICATION = "ui.wsgi.application"


# Database
# https://docs.djangoproject.com/en/4.2/ref/settings/#databases

DATABASES = {
    "default": {
        "ENGINE": "django.db.backends.sqlite3",
        "NAME": BASE_DIR / "db.sqlite3",
    }
}


# Password validation
# https://docs.djangoproject.com/en/4.2/ref/settings/#auth-password-validators

AUTH_PASSWORD_VALIDATORS = [
    {
        "NAME": "django.contrib.auth.password_validation.UserAttributeSimilarityValidator",
    },
    {
        "NAME": "django.contrib.auth.password_validation.MinimumLengthValidator",
    },
    {
        "NAME": "django.contrib.auth.password_validation.CommonPasswordValidator",
    },
    {
        "NAME": "django.contrib.auth.password_validation.NumericPasswordValidator",
    },
]


# Internationalization
# https://docs.djangoproject.com/en/4.2/topics/i18n/

LANGUAGE_CODE = "en-us"

TIME_ZONE = "Asia/Shanghai"
LOCAL_TZ = pytz.timezone(TIME_ZONE)
USE_I18N = True

USE_TZ = True


# Static files (CSS, JavaScript, Images)
# https://docs.djangoproject.com/en/4.2/howto/static-files/


# Default primary key field type
# https://docs.djangoproject.com/en/4.2/ref/settings/#default-auto-field

DEFAULT_AUTO_FIELD = "django.db.models.BigAutoField"


# 静态文件目录
STATIC_URL = "static/"
STATICFILES_DIRS = [
    Path.joinpath(BASE_DIR, "static"),
]

# Api文档设置
REST_FRAMEWORK = {
    # API文档方案
    "DEFAULT_SCHEMA_CLASS": "drf_spectacular.openapi.AutoSchema",
}

SPECTACULAR_SETTINGS = {
    "TITLE": "Garnet-ui API",
    "DESCRIPTION": "Garnet-ui API文档，使用 drf_spectacular 作为生成器",
    "VERSION": "0.0.1",
    "SERVE_INCLUDE_SCHEMA": False,
    # OTHER SETTINGS
}

# 文件上传
MEDIA_ROOT = Path.joinpath(BASE_DIR, "uploads")
MEDIA_URL = "uploads/"
# token有效期 7天
TOKEN_EXPIRES = 7


# django_q设置
Q_CLUSTER = {
    "name": "ui",
    "workers": 4,
    "recycle": 500,
    "timeout": 600,
    "retry": 600,
    "compress": True,
    "cpu_affinity": 1,
    "save_limit": 250,
    "queue_limit": 500,
    "label": "Django Q",
    "orm": "default",
    "queues": [
        {"name": "mpc_run", "workers": 1, "timeout": 90},
        {"name": "others", "workers": 4, "timeout": 60},
    ],
}
