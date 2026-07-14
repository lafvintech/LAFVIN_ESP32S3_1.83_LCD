# Configuration file for the Sphinx documentation builder.

from pygments.lexers import Python3Lexer
import time

project = "ESP32-S3 1.83-inch LCD Development Board"
copyright = f"{time.localtime().tm_year}, LAFVIN"
author = "LAFVIN"
source_encoding = "utf-8"

extensions = [
    "myst_parser",
    "sphinx_copybutton",
    "sphinxcontrib.video",
    "sphinxcontrib.images",
]

images_config = {
    "override_image_directive": False,
    "cache_path": "_images",
    "default_image_width": "100%",
    "default_image_height": "auto",
    "default_show_title": False,
    "download": True,
}

pygments_lexers = {
    "python-repl": Python3Lexer(),
}

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

templates_path = ["_templates"]
exclude_patterns = [
    "Tutorial/3.esp-idf.rst",
]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

# Add a project logo later, for example:
# html_logo = "_static/logo.png"

html_theme_options = {
    "logo_only": True,
}

