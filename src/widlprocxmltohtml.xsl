<?xml version="1.0" encoding="utf-8"?>
<!--====================================================================
$Id$
Copyright 2009 Aplix Corporation. All rights reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

XSLT stylesheet to convert widlprocxml into html documentation.
=====================================================================-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html" encoding="utf-8" indent="yes" doctype-public="html"/>

<xsl:param name="date" select="'error: missing date'"/>

<xsl:variable name="title" select="concat('The Bondi ',Definitions/Definition/*[1]/@identifier,' Module - Version ',/Definitions/Definition/Module/descriptive/version)"/>

<!-- section number of the Interfaces section. If there are typedefs, this is 3, otherwise 2 -->
<xsl:variable name="interfaces-section-number">
  <xsl:choose>
    <xsl:when test="/Definitions/Definition/Module/Definitions/Definition/Typedef">3</xsl:when>
    <xsl:otherwise>2</xsl:otherwise>
  </xsl:choose>
</xsl:variable>


<!--Root of document.-->
<xsl:template match="/">
    <html>
        <head>
            <link rel="stylesheet" type="text/css" href="widlhtml.css" media="screen"/>
            <title>
                <xsl:value-of select="$title"/>
            </title>
        </head>
        <body>
            <xsl:apply-templates/>
        </body>
    </html>
</xsl:template>

<!--Module: a whole API.-->
<xsl:template match="Module">
    <div class="api" id="{@fqid}">
        <a href="http://bondi.omtp.org"><img src="http://www.omtp.org/images/BondiSmall.jpg" alt="Bondi logo"/></a>
        <h1><xsl:value-of select="$title"/></h1>
        <h3>12 May 2009</h3>

        <h2>Authors</h2>
        <ul class="authors">
          <xsl:apply-templates select="descriptive/author"/>
        </ul>

        <p class="copyright"><small>© 2009 OMTP Ltd. All rights reserved. OMTP and OMTP BONDI are registered trademarks of OMTP Ltd.</small></p>

        <hr/>

        <h2>Abstract</h2>

        <xsl:apply-templates select="descriptive/brief"/>

        <h2>Table of Contents</h2>
        <ul class="toc">
          <li>1. <a href="#intro">Introduction</a>
          <ul>
            <xsl:if test="descriptive/def-api-feature">
              <li>1.1. <a href="#def-api-features">Features</a></li>
            </xsl:if>
            <xsl:if test="descriptive/def-device-cap">
              <li>1.2. <a href="#def-device-caps">Device Capabilities</a></li>
            </xsl:if>
          </ul>
          </li>
          <xsl:if test="Definitions/Definition/Typedef">
            <li>2. <a href="#typedefs">Type Definitions</a>
            <ul class="toc">
              <xsl:for-each select="Definitions/Definition/Typedef[TypedefRest/descriptive]">
                <li>2.<xsl:number value="position()"/>. <a href="#{@fqid}"><code><xsl:value-of select="TypedefRest/@identifier"/></code></a></li>
              </xsl:for-each>
            </ul>
            </li>
          </xsl:if>
          <li><xsl:number value="$interfaces-section-number"/>. <a href="#interfaces">Interfaces</a>
          <ul class="toc">
          <xsl:for-each select="Definitions/Definition[Interface/descriptive]">
            <li><xsl:number value="$interfaces-section-number"/>.<xsl:number value="position()"/>. <a href="#{@fqid}"><code><xsl:value-of select="Interface/@identifier"/></code></a></li>
          </xsl:for-each>
          </ul>
          </li>
        </ul>

        <hr/>

        <h2>Summary of Methods</h2>
        <xsl:call-template name="summary"/>
        
        <h2 id="intro">1. Introduction</h2>
        <xsl:apply-templates select="descriptive/description"/>

        <xsl:apply-templates select="descriptive/Code"/>

        <xsl:if test="descriptive/def-api-feature">
            <div id="def-api-features" class="def-api-features">
                <h3 id="features">1.1. Features</h3>
                <p>This is the list of URIs used to declare this API's features, for use in bondi.requestFeature. For each URL, the list of functions provided is provided.</p>
                <xsl:apply-templates select="descriptive/def-api-feature"/>
            </div>
        </xsl:if>
        <xsl:if test="descriptive/def-device-cap">
            <div class="def-device-caps" id="def-device-caps">
                <h3>1.2. Device capabilities</h3>
                <dl>
                  <xsl:apply-templates select="descriptive/def-device-cap"/>
                </dl>
            </div>
        </xsl:if>
        <xsl:if test="Definitions/Definition/Typedef">
            <div class="typedefs" id="typedefs">
                <h2>2. Type Definitions</h2>
                <xsl:apply-templates select="Definitions/Definition/Typedef[TypedefRest/descriptive]"/>
            </div>
        </xsl:if>
        <h2><xsl:value-of select="$interfaces-section-number"/>. Interfaces</h2>
        <xsl:apply-templates select="Definitions/Definition/Interface"/>
    </div>
</xsl:template>

<!--def-api-feature-->
<xsl:template match="def-api-feature">
      <dl class="def-api-feature">
          <dt><xsl:value-of select="@identifier"/></dt>
          <dd>
            <xsl:apply-templates select="descriptive/brief"/>
            <xsl:apply-templates select="descriptive"/>
            <xsl:apply-templates select="descriptive/Code"/>
            <xsl:if test="descriptive/device-cap">
              <p class="device-caps">Device capabilities: <code><xsl:apply-templates select="descriptive/device-cap/@identifier"/></code></p>
            </xsl:if>
          </dd>
      </dl>
</xsl:template>

<!--def-device-cap-->
<xsl:template match="def-device-cap">
    <dt class="def-device-cap"><code><xsl:value-of select="@identifier"/></code></dt>
    <dd>
      <xsl:apply-templates select="descriptive/brief"/>
      <xsl:apply-templates select="descriptive"/>
      <xsl:apply-templates select="descriptive/Code"/>
      <xsl:if test="descriptive/param">
        <div class="device-caps">
          <p>Security parameters:</p>
          <ul>
            <xsl:apply-templates select="descriptive/param"/>
          </ul>
        </div>
      </xsl:if>
    </dd>
</xsl:template>

<!--Exception: not implemented-->
<!--Valuetype: not implemented-->
<xsl:template match="Exception|Valuetype|Const">
    <xsl:if test="descriptive">
        <xsl:message terminate="yes">element <xsl:value-of select="name()"/> not supported</xsl:message>
    </xsl:if>
</xsl:template>

<!--Typedef.-->
<xsl:template match="Typedef[TypedefRest/descriptive]">
    <div class="typedef" id="{@fqid}">
        <h3>2.<xsl:number value="position()"/>. <code><xsl:value-of select="TypedefRest/@identifier"/></code></h3>
        <xsl:apply-templates select="TypedefRest/descriptive/brief"/>
        <xsl:apply-templates select="TypedefRest/descriptive/webidl"/>
        <xsl:apply-templates select="TypedefRest/descriptive"/>
        <xsl:apply-templates select="TypedefRest/descriptive/Code"/>
    </div>
</xsl:template>

<!--Interface.-->
<xsl:template match="Interface[descriptive]">
    <div class="interface" id="{@fqid}">
        <h3><xsl:value-of select="concat($interfaces-section-number,'.',1+count(../preceding::Definition[Interface]))"/>. <code><xsl:value-of select="@identifier"/></code></h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive/webidl"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
        <xsl:apply-templates select="*[name() != 'descriptive']"/>
    </div>
</xsl:template>
<xsl:template match="Interface[not(descriptive)]">
</xsl:template>

<xsl:template match="InterfaceInheritance/ScopedNameList">
              <p>
                <xsl:text>This interface inherits from: </xsl:text>
                <xsl:for-each select="ScopedName">
                  <code><xsl:value-of select="@identifier"/></code>
                  <xsl:if test="position!=last()">, </xsl:if>
                </xsl:for-each>
              </p>
</xsl:template>

<xsl:template match="InterfaceMembers">
    <xsl:if test="InterfaceMember/Const/descriptive">
        <div class="consts">
            <h4>Constants</h4>
            <dl>
              <xsl:apply-templates select="InterfaceMember/Const"/>
            </dl>
        </div>
    </xsl:if>
    <xsl:if test="InterfaceMember/Attribute/descriptive">
        <div class="attributes">
            <h4>Attributes</h4>
            <dl>
              <xsl:apply-templates select="InterfaceMember/Attribute"/>
            </dl>
        </div>
    </xsl:if>
    <xsl:if test="InterfaceMember/Operation/descriptive">
        <div class="methods">
            <h4>Methods</h4>
            <dl>
              <xsl:apply-templates select="InterfaceMember/Operation"/>
            </dl>
        </div>
    </xsl:if>
</xsl:template>

<!--Attribute-->
<xsl:template match="Attribute">
    <dt class="attribute" id="{@identifier}">
        <code>
            <xsl:if test="ReadOnly">
                [readonly]
            </xsl:if>
            <xsl:apply-templates select="DeclarationType"/>
            <xsl:value-of select="@identifier"/>
        </code></dt>
        <dd>
          <xsl:apply-templates select="descriptive/brief"/>
          <xsl:apply-templates select="descriptive"/>
          <xsl:apply-templates select="GetRaises[ExceptionList/ScopedNameList]"/>
          <xsl:apply-templates select="SetRaises[ExceptionList/ScopedNameList]"/>
          <xsl:apply-templates select="descriptive/Code"/>
        </dd>
</xsl:template>

<!--Const-->
<xsl:template match="Const">
  <dt class="const" id="{@fqid}">
    <code>
      <xsl:apply-templates select="DeclarationType"/>
      <xsl:value-of select="@identifier"/>
  </code>
  </dt>
  <dd>
    <xsl:apply-templates select="descriptive/brief"/>
    <xsl:apply-templates select="descriptive"/>
    <xsl:apply-templates select="descriptive/Code"/>
  </dd>
</xsl:template>

<!--Operation-->
<xsl:template match="Operation">
    <dt class="method" id="{concat(@identifier,generate-id(.))}">
        <code><xsl:value-of select="@identifier"/></code>
    </dt>
    <dd>
        <xsl:apply-templates select="descriptive/brief"/>
        <div class="synopsis">
            <h6>Signature</h6>
            <pre>
                <xsl:apply-templates select="ReturnType">
                    <xsl:with-param name="nodesc" select="1"/>
                </xsl:apply-templates>
                <xsl:text> </xsl:text>
                <xsl:value-of select="@identifier"/>
                <xsl:text>(</xsl:text>
                <xsl:apply-templates select="ArgumentList">
                    <xsl:with-param name="nodesc" select="1"/>
                </xsl:apply-templates>
                <xsl:text>);
</xsl:text></pre>
        </div>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="ArgumentList"/>
        <xsl:apply-templates select="ReturnType"/>
        <xsl:apply-templates select="Raises[ExceptionList/ScopedNameList]"/>
        <xsl:if test="descriptive/api-feature">
            <div class="api-features">
                <h6>API features</h6>
                <dl>
                    <xsl:apply-templates select="descriptive/api-feature"/>
                </dl>
            </div>
        </xsl:if>
        <xsl:apply-templates select="descriptive/Code"/>
    </dd>
</xsl:template>

<!--ReturnType. This is passed $nodesc=true to output just the return type,
    and not any documentation for it.-->
<xsl:template match="ReturnType">
    <xsl:param name="nodesc"/>
    <xsl:choose>
        <xsl:when test="$nodesc">
            <!--$nodesc is true: just output the type-->
            <xsl:apply-templates select="DeclarationType|void"/>
        </xsl:when>
        <xsl:otherwise>
            <!--$nodesc is false: output the documentation-->
            <xsl:if test="descriptive">
                <div class="returntype">
                    <h5>Return value</h5>
                    <xsl:apply-templates select="descriptive"/>
                </div>
            </xsl:if>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--ArgumentList. This is passed $nodesc=true to output just the argument
    types and names, and not any documentation for them.-->
<xsl:template match="ArgumentList">
    <xsl:param name="nodesc"/>
    <xsl:choose>
        <xsl:when test="$nodesc">
            <!--$nodesc is true: just output the types and names-->
            <xsl:apply-templates select="Argument[1]">
                <xsl:with-param name="nodesc" select="'nocomma'"/>
            </xsl:apply-templates>
            <xsl:apply-templates select="Argument[position() != 1]">
                <xsl:with-param name="nodesc" select="'comma'"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
            <!--$nodesc is false: output the documentation-->
            <div class="parameters">
                <h6>Parameters</h6>
                <ul>
                    <xsl:apply-templates/>
                </ul>
            </div>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--Argument. This is passed $nodesc=false to output the documentation,
    or $nodesc="nocomma" to output the type and name, or $nodesc="comma"
    to output a comma then the type and name. -->
<xsl:template match="Argument">
    <xsl:param name="nodesc"/>
    <xsl:choose>
        <xsl:when test="$nodesc">
            <!--$nodesc is true: just output the types and names-->
            <xsl:if test="$nodesc = 'comma'">
                <!--Need a comma first.-->
                <xsl:text>, </xsl:text>
            </xsl:if>
            <xsl:apply-templates select="DeclarationType"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="@identifier"/>
        </xsl:when>
        <xsl:otherwise>
            <!--$nodesc is false: output the documentation-->
            <li class="param">
                <xsl:value-of select="@identifier"/>:
                <xsl:apply-templates select="descriptive"/>
            </li>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--ExceptionList (for an Operation). It is already known that the list
    is not empty.-->
<xsl:template match="ExceptionList">
    <div class="exceptionlist">
        <h5>Exceptions</h5>
        <ul>
            <xsl:apply-templates/>
        </ul>
    </div>
</xsl:template>

<!--ScopedName, the name of an exception in an ExceptionList.-->
<xsl:template match="ExceptionList/ScopedNameList/ScopedName">
    <li class="exception">
        <xsl:value-of select="@identifier"/>:
        <xsl:apply-templates select="descriptive"/>
    </li>
</xsl:template>

<!--DeclarationType and its children.-->
<xsl:template match="DeclarationType">
    <xsl:apply-templates select="*"/>
</xsl:template>

<xsl:template match="UnsignedIntegerType">
    <xsl:apply-templates select="*"/>
</xsl:template>

<xsl:template match="IntegerType">
    <xsl:apply-templates select="*"/>
</xsl:template>

<xsl:template match="unsigned">
    <xsl:text>unsigned </xsl:text>
</xsl:template>

<xsl:template match="any|boolean|octet|float|short|long|Object|void">
    <xsl:value-of select="name()"/>
</xsl:template>

<xsl:template match="DOMString">
    <xsl:text>DOMString</xsl:text>
</xsl:template>

<xsl:template match="OptionalLong">
    <xsl:text>long </xsl:text>
</xsl:template>

<xsl:template match="ScopedName">
    <xsl:choose>
        <xsl:when test="@ref">
            <a href="{@ref}">
                <xsl:value-of select="@identifier"/>
            </a>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="@identifier"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>


<xsl:template match="descriptive[not(author)]">
  <xsl:apply-templates select="version"/>
  <xsl:if test="author">
  </xsl:if>
  <xsl:apply-templates select="description"/>
</xsl:template>

<!--brief-->
<xsl:template match="brief">
    <div class="brief">
        <p>
            <xsl:apply-templates/>
        </p>
    </div>
</xsl:template>

<!--description in ReturnType or Argument or ScopedName-->
<xsl:template match="ReturnType/descriptive/description|Argument/descriptive/description|ScopedName/descriptive/description">
    <!--If the description contains just a single <p> then we omit
        the <p> and just do its contents.-->
    <xsl:choose>
        <xsl:when test="p and count(*) = 1">
            <xsl:apply-templates select="p/*|p/text()"/>
        </xsl:when>
        <xsl:otherwise>
            <div class="description">
                <xsl:apply-templates/>
            </div>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--Other description-->
<xsl:template match="description">
    <div class="description">
        <xsl:apply-templates/>
    </div>
</xsl:template>

<!--Code-->
<xsl:template match="Code">
    <div class="example">
        <h5>Code example</h5>
        <pre class="examplecode"><xsl:apply-templates/></pre>
    </div>
</xsl:template>

<!--webidl : literal Web IDL from input-->
<xsl:template match="webidl">
    <pre class="webidl"><xsl:apply-templates/></pre>
</xsl:template>

<!--author-->
<xsl:template match="author">
    <li class="author"><xsl:apply-templates/></li>
</xsl:template>

<!--version-->
<xsl:template match="version">
    <div class="version">
        <h2>
            Version: <xsl:apply-templates/>
        </h2>
    </div>
</xsl:template>

<!--api-feature-->
<xsl:template match="api-feature">
    <dt>
        <xsl:value-of select="@identifier"/>
    </dt>
    <dd>
        <xsl:apply-templates/>
    </dd>
</xsl:template>

<!--param-->
<xsl:template match="param">
    <li>
        <code><xsl:value-of select="@identifier"/></code>:
        <xsl:apply-templates/>
    </li>
</xsl:template>

<!--html elements-->
<xsl:template match="a|b|br|dd|dl|dt|em|li|p|table|td|th|tr|ul">
    <xsl:element name="{name()}"><xsl:for-each select="@*"><xsl:attribute name="{name()}"><xsl:value-of select="."/></xsl:attribute></xsl:for-each><xsl:apply-templates/></xsl:element>
</xsl:template>

<xsl:template name="summary">
  <table class="summary">
    <thead>
      <tr><th>Interface</th><th>Method</th></tr>
    </thead>
    <tbody>
      <xsl:for-each select="Definitions/Definition[Interface/descriptive]">
        <tr><td><a href="#{Interface/@fqid}"><xsl:value-of select="Interface/@identifier"/></a></td>
        <td>
          <xsl:for-each select="Interface/InterfaceBody/InterfaceMembers/InterfaceMember/Operation">

            <xsl:apply-templates select="ReturnType">
                <xsl:with-param name="nodesc" select="1"/>
            </xsl:apply-templates>
            <xsl:text> </xsl:text>
            <a href="#{concat(@identifier,generate-id(.))}"><xsl:value-of select="@identifier"/></a>
            <xsl:text>(</xsl:text>
            <xsl:for-each select="ArgumentList/Argument">
              <xsl:variable name="type"><xsl:apply-templates select="DeclarationType"/></xsl:variable>
              <xsl:value-of select="concat(normalize-space($type),' ',@identifier)"/>
              <xsl:if test="position() != last()">, </xsl:if>
            </xsl:for-each>
            <xsl:text>)</xsl:text>
            <xsl:if test="position()!=last()"><br/></xsl:if>
          </xsl:for-each>
        </td>
        </tr>
      </xsl:for-each>
    </tbody>
  </table>
</xsl:template>

<!--<ref> element in literal Web IDL.-->
<xsl:template match="ref[@ref]">
    <a href="{@ref}">
        <xsl:apply-templates/>
    </a>
</xsl:template>

</xsl:stylesheet>

