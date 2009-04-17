<?xml version="1.0" encoding="us-ascii"?>
<!--====================================================================
$Id$
$URL$
Copyright 2009 Aplix Corporation. All rights reserved.

Stylesheet to extract DTD for widlprocxml from widlproc.html
=====================================================================-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="text" encoding="us-ascii" indent="no"/>

<!-- <pre class="dtd"> element -->
<xsl:template match="pre[@class='dtd']">
    <xsl:value-of select="."/>
</xsl:template>

<!--Ignore other text. -->
<xsl:template match="text()" priority="-100"/>

</xsl:stylesheet>
