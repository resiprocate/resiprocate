<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- $Revision: 1.1 $ -->

<!--
 This stylesheet is for converting docs to a single HTML page.
 -->


<xsl:import href="/usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl"/>
<!-- There IS a way to control the chunk output file names ... use it -->

<xsl:param name="html.stylesheet">design.css</xsl:param>

<xsl:param name="shade.verbatim" select="1"/>
<xsl:param name="shade.verbatim.style">
  <xsl:attribute name="border">0</xsl:attribute>
  <xsl:attribute name="bgcolor">#ffcc80</xsl:attribute>
  <xsl:attribute name="width">80%</xsl:attribute>
</xsl:param>

<xsl:param name="section.autolabel" select="1"/>

<xsl:param name="formal.title.placement">
  figure after
  example after
  equation after
  table after
</xsl:param>

<xsl:param name="linenumbering.extension" select="1"/>
<xsl:param name="use.extensions" select="1"/>

</xsl:stylesheet>
<!--
Local Variables: ***
mode:XSL ***
End: ***
-->
