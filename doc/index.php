<?php

$domain=ereg_replace('[^\.]*\.(.*)$','\1',$_SERVER['HTTP_HOST']);
$group_name=ereg_replace('([^\.]*)\..*$','\1',$_SERVER['HTTP_HOST']);

echo '<?xml version="1.0" encoding="UTF-8"?>';
?>
<!DOCTYPE html
	PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en   ">
<link href="doc/doxygen.css" rel="stylesheet" type="text/css">
<link href="doc/tabs.css" rel="stylesheet" type="text/css">

  <head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<title><?php echo $project_name; ?></title>
	<script language="JavaScript" type="text/javascript">
	<!--
	function help_window(helpurl) {
		HelpWin = window.open( helpurl,'HelpWindow','scrollbars=yes,resizable=yes,toolbar=no,height=400,width=400');
	}
	// -->
		</script>

<style type="text/css">
	<!--
	BODY {
		margin-top: 3;
		margin-left: 3;
		margin-right: 3;
		margin-bottom: 3;
		background: #5651a1;
	}
	ol,ul,p,body,td,tr,th,form { font-family: verdana,arial,helvetica,sans-serif; font-size:small;
		color: #333333; }

	h1 { font-size: x-large; font-family: verdana,arial,helvetica,sans-serif; }
	h2 { font-size: large; font-family: verdana,arial,helvetica,sans-serif; }
	h3 { font-size: medium; font-family: verdana,arial,helvetica,sans-serif; }
	h4 { font-size: small; font-family: verdana,arial,helvetica,sans-serif; }
	h5 { font-size: x-small; font-family: verdana,arial,helvetica,sans-serif; }
	h6 { font-size: xx-small; font-family: verdana,arial,helvetica,sans-serif; }

	pre,tt { font-family: courier,sans-serif }

	a:link { text-decoration:none }
	a:visited { text-decoration:none }
	a:active { text-decoration:none }
	a:hover { text-decoration:underline; color:red }

	.titlebar { color: black; text-decoration: none; font-weight: bold; }
	a.tablink { color: black; text-decoration: none; font-weight: bold; font-size: x-small; }
	a.tablink:visited { color: black; text-decoration: none; font-weight: bold; font-size: x-small; }
	a.tablink:hover { text-decoration: none; color: black; font-weight: bold; font-size: x-small; }
	a.tabsellink { color: black; text-decoration: none; font-weight: bold; font-size: x-small; }
	a.tabsellink:visited { color: black; text-decoration: none; font-weight: bold; font-size: x-small; }
	a.tabsellink:hover { text-decoration: none; color: black; font-weight: bold; font-size: x-small; }
	-->
</style>

</head>

<body>
<table border="0" width="100%" cellspacing="0" cellpadding="0">

	<tr>
		<td style="text-align: left; vertical-align: top;"><a href="http://<?php echo $domain; ?>/projects/<?php echo $group_name; ?>"><img
		  src="simgrid_logo_small2.png"
		  style="width: 220px; height: 54px; text-decoration: underline;" border="0" alt="SimGrid"></a><br>
		</td>
		<td style="text-align: right; vertical-align: top;"><a href="http://<?php echo $domain; ?>"><img src="http://<?php echo $domain; ?>/themes/inria/images/logo.png" border="0" alt="gforge INRIA" width="198" height="52" /></a></td>
	</tr>
</table>
<div class="tabs">
  <ul>
    <li><a href="doc/index.html"><span>Main&nbsp;Page</span></a></li>       <li><a href="doc/faq.html"><span>FAQ</span></a></li>
    <li><a href="doc/modules.html"><span>Modules API</span></a></li>
    <li><a href="doc/publis.html"><span>Publications</span></a></li>
    
    <li><a href="doc/pages.html"><span>Related&nbsp;Pages</span></a></li>
  </ul></div>
<center>  
 </center><p><br>
<table border="0" width="100%" cellspacing="0" cellpadding="0">

	<tr>
		<td>&nbsp;</td>
		<td colspan="3">



		<!-- start tabs -->

	<tr>
		<td align="left" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/topleft.png" height="9" width="9" alt="" /></td>
		<td bgcolor="#E0E0E0" width="30"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="30" height="1" alt="" /></td>
		<td bgcolor="#E0E0E0"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="1" height="1" alt="" /></td>
		<td bgcolor="#E0E0E0" width="30"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="30" height="1" alt="" /></td>
		<td align="right" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/topright.png" height="9" width="9" alt="" /></td>
	</tr>

	<tr>

		<!-- Outer body row -->

		<td bgcolor="#E0E0E0"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="10" height="1" alt="" /></td>
		<td valign="top" width="99%" bgcolor="#E0E0E0" colspan="3">

			<!-- Inner Tabs / Shell -->

			<table border="0" width="100%" cellspacing="0" cellpadding="0">
			<tr>
				<td align="left" bgcolor="#ffffff" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/topleft-inner.png" height="9" width="9" alt="" /></td>
				<td bgcolor="#ffffff"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="1" height="1" alt="" /></td>
				<td align="right" bgcolor="#ffffff" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/topright-inner.png" height="9" width="9" alt="" /></td>
			</tr>

			<tr>
				<td bgcolor="#ffffff"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="10" height="1" alt="" /></td>
				<td valign="top" width="99%" bgcolor="white">

	<!-- whole page table -->
<table width="100%" cellpadding="5" cellspacing="0" border="0">
<tr><td width="65%" valign="top">
<?php if ($handle=fopen('http://'.$domain.'/export/projtitl.php?group_name='.$group_name,'r')){
$contents = '';
while (!feof($handle)) {
	$contents .= fread($handle, 8192);
}
$contents=str_replace("Welcome to", "Welcome to the", $contents);
fclose($handle);
echo $contents; } ?>

The specific goal of the project is to facilitate research in the area of
distributed and parallel application scheduling on distributed computing
platforms ranging from simple network of workstations to Computational
Grids.

<h2><a class="anchor" name="overview">
Overview of the toolkit components</a></h2>
As depicted by the following diagram, the SimGrid toolkit is basically three-layered (click on the picture to jump to a specific component).<p>
 
<center>
 <IMG style="border:0px "SRC="doc/simgrid_modules.png" USEMAP="#simgrid_modules">
<MAP NAME="simgrid_modules">
<AREA SHAPE="poly" COORDS="418,171,418,139,343,139,343,89,239,89,239,171" href="doc/group__GRAS__API.html" ALT="GRAS">
<AREA COORDS="428,89,607,171" href="doc/group__SMPI__API.html" ALT="SMPI">
<AREA COORDS="50,89,229,171" href="doc/group__MSG__API.html" ALT="MSG">
<AREA COORDS="50,180,607,240" href="doc/group__SURF__API.html" ALT="SMPI">
<AREA SHAPE="poly" COORDS="43,89,12,89,12,313,607,313,607,250,43,250" href="doc/group__XBT__API.html" ALT="XBT">
<AREA COORDS="347,89,417,133" href="doc/group__AMOK__API.html" ALT="AMOK">
</MAP>
  
<br><b>Relationships between the SimGrid components</b>
</center>
<h3><a class="anchor" name="overview_fondation">
Base layer</a></h3>
The base of the whole toolkit is constituted by the <b><a class="el" href="doc/group__XBT__API.html">XBT</a> (eXtended Bundle of Tools)</b>.<p>
It is a portable library providing some grounding features such as <a class="el" href="doc/group__XBT__log.html">Logging support</a>, <a class="el" href="doc/group__XBT__ex.html">Exception support</a> and <a class="el" href="doc/group__XBT__config.html">Configuration support</a>. XBT also encompass the following convenient datastructures: <a class="el" href="doc/group__XBT__dynar.html">Dynar: generic dynamic array</a>, <a class="el" href="doc/group__XBT__fifo.html">Fifo: generic workqueue</a>, <a class="el" href="doc/group__XBT__dict.html">Dict: generic dictionnary</a>, <a class="el" href="doc/group__XBT__heap.html">Heap: generic heap data structure</a>, <a class="el" href="doc/group__XBT__set.html">Set: generic set datatype</a> and <a class="el" href="doc/group__XBT__swag.html">Swag: O(1) set datatype</a>.<p>
See the <a class="el" href="doc/group__XBT__API.html">XBT</a> section for more details.<h3><a class="anchor" name="overview_kernel">
Simulation kernel layer</a></h3>
The core functionnalities to simulate a virtual platform are provided by a module called <b><a class="el" href="doc/group__SURF__API.html">SURF</a></b> ("that's historical, my friend"). It is very low-level and is not intended to be used as such by end-users. Instead, it serve as a basis for the higher level layer.<p>
SURF main features are a fast max-min linear solver and the ability to change transparently the model used to describe the platform. This greatly eases the comparison of the several models existing in the litterature.<p>
See the <a class="el" href="doc/group__SURF__API.html">SURF</a> section for more details.<h3><a class="anchor" name="overview_envs">
Programmation environments layer</a></h3>
This simulation kernel is used to build several programmation environments. Each of them target a specific audiance and constitute a different paradigm. To choose which of them you want to use, you have to think about what you want to do and what would be the result of your work.<p>
<ul>
<li>If you want to study a theoritical problem and compare several heuristics, you probably want to try <b><a class="el" href="doc/group__MSG__API.html">MSG</a></b> (yet another historical name). It was designed exactly to that extend and should allow you to build easily rather realistic multi-agents simulation. Yet, realism is not the main goal of this environment and the most annoying technical issues of real platforms are masked here. Check the <a class="el" href="doc/group__MSG__API.html">MSG</a> section for more information.</li></ul>
<p>
<ul>
<li>If you want to study the behaviour of a MPI application using emulation technics, you should have a look at the <b><a class="el" href="doc/group__SMPI__API.html">SMPI</a></b> (Simulated MPI) programming environment. Unfortunately, this work is still underway. Check the <a class="el" href="doc/group__SMPI__API.html">SMPI</a> section for more information.</li></ul>
<p>
<ul>
<li>If you want to develop a real distributed application, then you may find <b><a class="el" href="doc/group__GRAS__API.html">GRAS</a></b> (Grid Reality And Simulation) useful. This is an API for the realization of distributed applications. <br>
<br>
 Moreover, there is two implementations of this API: one on top of the SURF (allowing to develop and test your application within the comfort of the simulator) and another suited for deployment on real platforms (allowing the resulting application to be highly portable and extremely efficient). <br>
<br>
 Even if you do not plan to run your code for real, you may want to switch to GRAS if you intend to use MSG in a very intensive way (e.g. for simulating a peer-to-peer environment). <br>
<br>
 See the <a class="el" href="doc/group__GRAS__API.html">GRAS</a> section for more details.</li></ul>
<p>
If your favorite programming environment/model is not there (BSP, components, etc.) is not represented in the SimGrid toolkit yet, you may consider adding it. You should contact us first on the <a href="doc/http://lists.gforge.inria.fr/mailman/listinfo/simgrid-devel>">SimGrid developers mailing list</a>, though.<p>
Any question, remark or suggestion are welcome on the <a href="doc/http://lists.gforge.inria.fr/mailman/listinfo/simgrid-user>">SimGrid users mailing list</a>.<p>
<hr>

<?php if ($handle=fopen('http://'.$domain.'/export/projnews.php?group_name='.$group_name,'r')){
$contents = '';
while (!feof($handle)) {
	$contents .= fread($handle, 8192);
}
fclose($handle);
$contents=str_replace('href="/','href="http://'.$domain.'/',$contents);
echo $contents; } ?>

</td>

<td width="35%" valign="top">

		<table cellspacing="0" cellpadding="1" width="100%" border="0" bgcolor="#d5d5d7">
		<tr><td>
			<table cellspacing="0" cellpadding="2" width="100%" border="0" bgcolor="#eaecef">
				<tr style="background-color:#d5d5d7" align="center">
					<td colspan="2"><span class="titlebar">Project Summary</span></td>
				</tr>
				<tr align="left">
					<td colspan="2">

<?php if($handle=fopen('http://'.$domain.'/export/projhtml.php?group_name='.$group_name,'r')){
$contents = '';
while (!feof($handle)) {
	$contents .= fread($handle, 8192);
}
fclose($handle);
$contents=str_replace('href="/','href="http://'.$domain.'/',$contents);
$contents=str_replace('src="/','src="http://'.$domain.'/',$contents);
echo $contents; } ?>

					</td>
				</tr>
			</table>
		</td></tr>
		</table><p>&nbsp;</p>
</td></tr></table>
			&nbsp;<p>
                        <center>
                                Help: <a href="mailto:siteadmin-help@lists.gforge.inria.fr">siteadmin-help@lists.gforge.inria.fr</a> Webmaster: <a href="mailto:webmaster@gforge.inria.fr">webmaster@gforge.inria.fr</a>
                        </center>
			<!-- end main body row -->


				</td>
				<td width="10" bgcolor="#ffffff"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="2" height="1" alt="" /></td>
			</tr>
			<tr>
				<td align="left" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/bottomleft-inner.png" height="11" width="11" alt="" /></td>
				<td bgcolor="#ffffff"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="1" height="1" alt="" /></td>
				<td align="right" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/bottomright-inner.png" height="11" width="11" alt="" /></td>
			</tr>
			</table>

		<!-- end inner body row -->

		</td>
		<td width="10" bgcolor="#E0E0E0"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="2" height="1" alt="" /></td>
	</tr>
	<tr>
		<td align="left" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/bottomleft.png" height="9" width="9" alt="" /></td>
		<td bgcolor="#E0E0E0" colspan="3"><img src="http://<?php echo $domain; ?>/themes/inria/images/clear.png" width="1" height="1" alt="" /></td>
		<td align="right" bgcolor="#E0E0E0" width="9"><img src="http://<?php echo $domain; ?>/themes/inria/images/tabs/bottomright.png" height="9" width="9" alt="" /></td>
	</tr>
</table>

<!-- PLEASE LEAVE "Powered By Gforge" on your site -->
<br />
<center>
<a href="http://gforge.org/"><img src="http://gforge.org/images/pow-gforge.png" alt="Powered By GForge Collaborative Development Environment" border="0" /></a>
</center>


</body>
</html>
