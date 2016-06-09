@pushd ..\src
del ..\microprofile_html.h
embed ../microprofile_html.h microprofile.html ____embed____ g_MicroProfileHtml MICROPROFILE_EMBED_HTML
	
embed ../microprofile_html.h microprofilelive.html ____embed____ g_MicroProfileHtmlLive MICROPROFILE_EMBED_HTML


@popd
@echo "done embedding"