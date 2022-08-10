Table of Contents
=================

  * [SmartMet Server](#SmartMet Server)
  * [Introduction](#introduction)
  * [Admin plugin: Services](#admin-plugin-services)
    * [Cluster information](#cluster-information)
    * [Backends](#backends)
    * [QEngine](#qengine)
    * [Service access information](#service-access-information)
    * [GeoEngine cache status](#geoengine-cache-status)
    * [GeoEngine reload](#geoengine-reload)

# SmartMet Server

[SmartMet Server](https://github.com/fmidev/smartmet-server) is a data and product server for MetOcean data. It
provides a high capacity and high availability data and product server
for MetOcean data. The server is written in C++, since 2008 it has
been in operational use by the Finnish Meteorological Institute FMI.

# Introduction

SmartMet plugin admin provides status and administration services. It can provide the cluster information regarding the frontend and backend servers. Admin plugin can give the information about the services that can be provided by  a particular backend server and also the names of the backend servers which provide a particular service etc.

Since the front ends delegate the requests to a random background
machine, the plugin is usually called for a dedicated machine as given
in the examples below. In the examples Server1 and Server2 are the
servers for which the information is sought. The admin plugin queries
described in this section should not be directly accessed by users.

<pre><code>
http://data.fmi.fi/Server1/admin?what=<task>
http://data.fmi.fi/Server2/admin?what=<task>
</code></pre>

# Admin plugin: Services

The admin plugin  provides administration  information about the server state such as:
 
* Cluster information
* Known backends 
* Service access information
* Available qengine data
* geoengine cache status
* geoengine reload

## Cluster information
The cluster status information can be requested both from the frontends and the backends. However, one cannot choose the frontend, since the selection is done by the load balancer.

The following are some example requests:
* The request to get the broadcast cluster status information at the frontend
<pre><code>
 http://data.fmi.fi/admin?what=clusterinfo
</code></pre>

The result of this request consists of the server information and the services known by the frontend server FServer

<pre><code>
<b>Broadcast Cluster Information </b>
This server is a FRONTEND 
* Host: FServer
* Comment: SmartMet server in FServer
* HTTP interface: Host IP:port
* Throttle Limit: 0
* Broadcast Interface: Broadcast IP address

<b> Services known by the frontend server </b>
* URI's of different services and plugins
</code></pre>

* The request to get the backend status information for backend server  BServer
<pre><code>
 http://data.fmi.fi/BServer/admin?what=clusterinfo
</code></pre>

The result of this request consists of the server information and the services currently provided by this backend server
<pre><code>
<b>Broadcast Cluster Information </b>
This server is a BACKEND
* Host: BServer
* Comment: BServer is a SmartMet server backend
* HTTP interface: Host IP:port
* Throttle Limit: 0
* Broadcast Interface: Broadcast IP address

Services currently provided by this backend server
    /
    /admin
    /autocomplete
    /avi
    /csection
    /dali
    /download
    /favicon.ico
    /flashplugin
    /meta
    /observe
    /salami
    /textgen
    /timeseries
    /trajectory
    /wfs
    /wfs/eng
    /wfs/fin
    /wms
</code></pre>

Note that the first request for cluster status is not handled by the admin plugin, since the admin plugins are installed only in backend machines. The first request is handled by the frontend plugin.

## Service information
The service status information can be requested the backends.

* The request to get the backend status information for backend server  BServer
<pre><code>
 http://data.fmi.fi/BServer/admin?what=serviceinfo
</code></pre>

The result of this request consists of the services currently provided by this server
<pre><code>
<b>Services currently provided by this server
    /
    /admin
    /autocomplete
    /avi
    /csection
    /dali
    /download
    /favicon.ico
    /flashplugin
    /meta
    /observe
    /salami
    /textgen
    /timeseries
    /trajectory
    /wfs
    /wfs/eng
    /wfs/fin
    /wms
</code></pre>

## Backends
In the SmartMet server environment,  the frontends know what services the backends provide. One can request for either the full list of backends or just those that provide a  given service. The output includes the names of the backends plus the IP address including the port. The output format can be selected.

The following are some example requests:

* The request to get the information regarding all backend servers 
<pre><code>
 http://data.fmi.fi/admin?what=backends&format=debug
</code></pre>

The result of this request can be in the following format:

| Backend Server name | IP address | Port |
|-------|------------|-----|

* The backends with autocomplete service

<pre><code>
 http://data.fmi.fi/admin?what=backends&service=autocomplete&format=debug
</code></pre>

The result of this request can be in the following format:

| Backend Server name | IP address | Port |
|-------|------------|-----|


##QEngine

QEngine maintains the QueryData in memory. The admin-queries can be used to obtain the information about the currently loaded QueryData. For backends the current list of loaded files in server Server1 can be obtained as follows:

<pre><code>
 server_name: http://brainstormgw.fmi.fi/Server1/admin?what=qengine 
</code></pre>

Projection output defaults to newbase form. If WKT form is needed, supply additional &projformat=wkt.

Usually one is interested in what data is currently loaded in ALL backends which are visible to the given frontend. For this, a similar query can be performed to a frontend-server:

<pre><code>
 http://data.fmi.fi/admin?what=qengine
</code></pre>

This output shows the files which are currently available in all backends providing the timeseries - service. One can refine the search to include only files (producers) which provide given parameters:


<pre><code>
 http://data.fmi.fi/admin?what=qengine&param=Pressure,Icing
</code></pre>

The result obtained may be as given below:

|Producer| Path | OriginTime |
|--------|------|-----------|
|ecmwf_world_level| /smartmet/data/ecmwf/world/level/querydata/201611160819_ecmwf_world_level.sqd |2016-11-16 00:00:00|
|ecmwf_europe_level |/smartmet/data/ecmwf/europe/level_xh/querydata/201611160726_ecmwf_europe_level.sqd|2016-11-16 00:00:00|

The above query shows producers which provide both Pressure and Icing - parameters, sorted in descending order by file origin time.

Newbase id numbers are also supported, the search above is identical to:

<pre><code>
http://data.fmi.fi/admin?what=qengine&type=id&param=480,1
</code></pre>

Keep in mind  that the actual frontend given by data.fmi.fi is unpredictable. This is not an issue as long as all frontends serve the same backends.

##Service access information
SmartMet  server optionally logs statistics for successfully handled requests. Access to this functionality is available through the query in which one has to specify the name of the server:

<pre><code>
 Server1:  http://brainstormgw.fmi.fi/Server1/admin?what=services
</code></pre>

This functionality is disabled by default, to enable it run the following query:
<pre><code>
 Server1:  http://data.fmi.fi/Server1/admin?what=services&logging=enable
</code></pre>

Similarly, statistics collection can be disabled by replacing "enable" with "disable". Currently, requests older than one week are not considered.

##GeoEngine cache status
GeoEngine caches all requests it has made into the MySQL database, and keeps a count on how many times the information has been accessed from the cache.
The information is reset if a reload request is made.

A sample request for Server1 is given below

<pre><code>
 Server1:  http://data.fmi.fi/Server1/admin?what=geonames
</code></pre>

The result of this query can be as follows:

|StartTime| Uptime | LastReload | CacheMaxSize | NameSearchRate | NameSearches | CoordinateSearchRate |	CoordinateSearches | GeoidSearchRate | GeoidSearches |	KeywordSearchRate | KeywordSearches | AutocompleteSearchRate |	AutocompleteSearches |
|-----|------|------|------|-----|-----|-----|------|------|------|-------|----|---|-----|
|2016-Nov-15  16:15:26	|23:39:22 |	2016-Nov-16 10:56:38 |	10000000 |	18.1449/sec, 1088.69/min |	1545254	| 39.2175/sec, 2353.05/min |	3339838|	39.8747/sec, 2392.48/min |	3395809|	0.295824/sec, 17.7495/min|	25193 |	1.36237/sec,| 81.7421/min|	116022|

##GeoEngine reload
GeoEngine can reload the geonames database in a separate thread, and quickly swap all the information into use, replacing all the previously downloaded data. The swap will be delayed by active read requests until the write operation gains a lock on the necessary data structures.

Note that the reload request will  use one thread from the server until the swap has been completed. If you are running a debug server with only one thread, you will not be able to do any requests while the reload is in progress.

<pre><code>
 Server1:  http://data.fmi.fi/Server1/admin?what=reload
</code></pre>

Possible responses from the server are:

    1. GeoEngine is not available
    2. GeoEngine reload refused, one is already in progress
    3. GeoEngine reloaded in N seconds

It is also possible than an error occurs during the reload, for example if the MySQL server has gone down. In that case the old data structures remain active and no data is lost.
