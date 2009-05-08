<?xml version="1.0" encoding="us-ascii"?>
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
<xsl:output method="html" encoding="us-ascii" indent="yes"/>

<!--Root of document.-->
<xsl:template match="/">
    <html>
        <head>
            <link rel="stylesheet" type="text/css" href="widlhtml.css" media="screen"/>
            <title>
                <xsl:value-of select="Definitions/Definition/*[1]/@identifier"/>
            </title>
        </head>
        <body>
            <xsl:apply-templates/>
        </body>
    </html>
</xsl:template>

<!--Module: a whole API.-->
<xsl:template match="Module">
    <div class="api">
        <h1><xsl:value-of select="@identifier"/></h1>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
        <xsl:if test="descriptive/def-api-feature">
            <div class="def-api-features">
                <h3>API features</h3>
                <xsl:apply-templates select="descriptive/def-api-feature"/>
            </div>
        </xsl:if>
        <xsl:if test="descriptive/def-device-cap">
            <div class="def-device-caps">
                <h2>Device capabilities</h2>
                <xsl:apply-templates select="descriptive/def-device-cap"/>
            </div>
        </xsl:if>
        <xsl:if test="Definitions/Definition/Typedef[TypedefRest/descriptive]">
            <div class="typedefs">
                <h2>Typedefs</h2>
                <xsl:apply-templates select="Definitions/Definition/Typedef[TypedefRest/descriptive]"/>
            </div>
        </xsl:if>
        <xsl:apply-templates select="Definitions/Definition/Interface"/>
    </div>
</xsl:template>

<!--def-api-feature-->
<xsl:template match="def-api-feature">
    <div class="def-api-feature">
        <h3><xsl:value-of select="@identifier"/></h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
        <xsl:if test="descriptive/device-cap">
            <div class="device-caps">
                <h4>Device capabilities used</h4>
                <ul>
                    <xsl:apply-templates select="descriptive/device-cap"/>
                </ul>
            </div>
        </xsl:if>
    </div>
</xsl:template>

<!--def-device-cap-->
<xsl:template match="def-device-cap">
    <div class="def-device-cap">
        <h3><xsl:value-of select="@identifier"/></h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
        <xsl:if test="descriptive/param">
            <div class="device-caps">
                <h4>Security parameters</h4>
                <ul>
                    <xsl:apply-templates select="descriptive/param"/>
                </ul>
            </div>
        </xsl:if>
    </div>
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
    <div class="typedef">
        <h3><xsl:value-of select="TypedefRest/@identifier"/></h3>
        <xsl:apply-templates select="TypedefRest/descriptive/brief"/>
        <xsl:apply-templates select="TypedefRest/descriptive/webidl"/>
        <xsl:apply-templates select="TypedefRest/descriptive"/>
        <xsl:apply-templates select="TypedefRest/descriptive/Code"/>
    </div>
</xsl:template>

<!--Interface.-->
<xsl:template match="Interface[descriptive]">
    <div class="interface">
        <h1><xsl:value-of select="@identifier"/></h1>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive/webidl"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
        <xsl:apply-templates select="*[name() != 'descriptive']"/>
    </div>
</xsl:template>
<xsl:template match="Interface[not(descriptive)]">
</xsl:template>

<xsl:template match="InterfaceMembers">
    <xsl:if test="InterfaceMember/Const/descriptive">
        <div class="consts">
            <h2>Constants</h2>
            <xsl:apply-templates select="InterfaceMember/Const"/>
        </div>
    </xsl:if>
    <xsl:if test="InterfaceMember/Const/descriptive">
        <div class="consts">
            <h2>Constants</h2>
            <xsl:apply-templates select="InterfaceMember/Const"/>
        </div>
    </xsl:if>
    <xsl:if test="InterfaceMember/Attribute/descriptive">
        <div class="attributes">
            <h2>Attributes</h2>
            <xsl:apply-templates select="InterfaceMember/Attribute"/>
        </div>
    </xsl:if>
    <xsl:if test="InterfaceMember/Operation/descriptive">
        <div class="methods">
            <h2>Methods</h2>
            <xsl:apply-templates select="InterfaceMember/Operation"/>
        </div>
    </xsl:if>
</xsl:template>

<!--Attribute-->
<xsl:template match="Attribute">
    <div class="attribute">
        <h3>
            <xsl:if test="ReadOnly">
                [readonly]
            </xsl:if>
            <xsl:apply-templates select="DeclarationType"/>
            <xsl:value-of select="@identifier"/>
        </h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="GetRaises[ExceptionList/ScopedNameList]"/>
        <xsl:apply-templates select="SetRaises[ExceptionList/ScopedNameList]"/>
        <xsl:apply-templates select="descriptive/Code"/>
    </div>
</xsl:template>

<!--Const-->
<xsl:template match="Const">
    <div class="const">
        <h3>
            <xsl:apply-templates select="DeclarationType"/>
            <xsl:value-of select="@identifier"/>
        </h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="descriptive/Code"/>
    </div>
</xsl:template>

<!--Operation-->
<xsl:template match="Operation">
    <div class="method">
        <h3>
            <xsl:value-of select="@identifier"/>
        </h3>
        <xsl:apply-templates select="descriptive/brief"/>
        <div class="synopsis">
            <h4>Synopsis</h4>
            <code>
                <xsl:apply-templates select="ReturnType">
                    <xsl:with-param name="nodesc" select="1"/>
                </xsl:apply-templates>
                <xsl:text> </xsl:text>
                <xsl:value-of select="@identifier"/>
                <xsl:text>(</xsl:text>
                <xsl:apply-templates select="ArgumentList">
                    <xsl:with-param name="nodesc" select="1"/>
                </xsl:apply-templates>
                <xsl:text>);</xsl:text>
            </code>
        </div>
        <xsl:apply-templates select="descriptive"/>
        <xsl:apply-templates select="ArgumentList"/>
        <xsl:apply-templates select="ReturnType"/>
        <xsl:apply-templates select="Raises[ExceptionList/ScopedNameList]"/>
        <xsl:if test="descriptive/api-feature">
            <div class="api-features">
                <h4>Api features</h4>
                <dl>
                    <xsl:apply-templates select="descriptive/api-feature"/>
                </dl>
            </div>
        </xsl:if>
        <xsl:apply-templates select="descriptive/Code"/>
    </div>
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
                    <h4>Return value</h4>
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
                <h4>Parameters</h4>
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
        <h4>Exceptions</h4>
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
<xsl:template match="any|boolean|octet|float|short|long|Object|unsigned|void">
    <xsl:value-of select="name()"/>
</xsl:template>

<xsl:template match="DOMString">
    <xsl:text>string</xsl:text>
</xsl:template>

<xsl:template match="OptionalLong">
    long
</xsl:template>
    
<xsl:template match="ScopedName">
    <xsl:if test="ScopedNameAfterColon">
        <xsl:text>::</xsl:text>
    </xsl:if>
    <xsl:value-of select="@identifier"/>
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="ScopedNameParts">
    <xsl:text>::</xsl:text>
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="ScopedNameAfterColon|ScopedNamePartsAfterColon">
    <xsl:value-of select="@identifier"/>
    <xsl:apply-templates/>
</xsl:template>

<!--descriptive. This does not output any brief or Code or api-feature
    or webidl element; they are handled by descriptive's parent.-->
<xsl:template match="descriptive">
    <xsl:apply-templates select="description"/>
    <xsl:if test="author">
        <p class="authors">
            Authors:
            <xsl:apply-templates select="author"/>
        </p>
    </xsl:if>
    <xsl:apply-templates select="version"/>
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
        <h4>Description</h4>
        <xsl:apply-templates/>
    </div>
</xsl:template>

<!--Code-->
<xsl:template match="Code">
    <div class="example">
        <h4>Code example</h4>
        <pre class="examplecode"><xsl:apply-templates/></pre>
    </div>
</xsl:template>

<!--webidl : literal Web IDL from input-->
<xsl:template match="webidl">
    <pre class="webidl"><xsl:value-of select="text()"/></pre>
</xsl:template>

<!--author-->
<xsl:template match="author">
    <div class="author">
        <br/>
        <xsl:apply-templates/>
    </div>
</xsl:template>

<!--version-->
<xsl:template match="version">
    <div class="version">
        <p>
            Version: <xsl:apply-templates/>
        </p>
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

<!--device-cap-->
<xsl:template match="device-cap">
    <li>
        <xsl:value-of select="@identifier"/>:
        <xsl:apply-templates/>
    </li>
</xsl:template>

<!--param-->
<xsl:template match="param">
    <li>
        <xsl:value-of select="@identifier"/>:
        <xsl:apply-templates/>
    </li>
</xsl:template>

<!--html elements-->
<xsl:template match="a|b|br|dd|dl|dt|em|li|p|table|td|th|tr|ul">
    <xsl:element name="{name()}"><xsl:for-each select="@*"><xsl:attribute name="{name()}" select="."/></xsl:for-each><xsl:apply-templates/></xsl:element>
</xsl:template>

</xsl:stylesheet>
