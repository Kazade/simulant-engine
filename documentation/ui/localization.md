# Localisation

Simulant comes with a built-in method of managing translations and languages.

## Translations

Simulant maintains an "active language" at all times. By default this is en_US, and it is assumed that all text strings in the source codes
are en_US by default. If the source text of your translations is in a different language you can override the `source_language_code` in the `AppConfig`.

If you want to activate a different language than the source language, you must call `activate_language(language_code)`, this will search the available asset
paths for an ARB file (Flutter translation format) with the name of the language code. For example, `get_app()->activate_language("fr");` will search for a file
names `fr.arb` and load the translations from there.

From this point on, if you want to retrieve the translated text for a source text, you can call `get_app()->translated_text("my text");` and this will return the
equivalent as loaded from the ARB file if that text exists. If the text doesn't exist in the ARB file then the original source text will be returned.

`get_app()->translated_text()` is conveniently aliased to the `_T()` macro, so it's much easier to use, for example:

```
auto my_label = stage->ui->new_widget_as_label(_T("This is some translateable text"));
```

It is highly recommended that you use this macro for all translations. This is so that automated tools can easily find your translatable strings by searching the code for
the `_T()` macro.

## Limitations

 - Currently placeholders and plurals are unimplemented
 - Entries in the ARB files are required to specify their "source_text" attribute
