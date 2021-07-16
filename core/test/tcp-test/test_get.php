<?php
curl_get("http://localhost:9876/service/httpDemo?fpnn={\"key\":5,\"key2\":\"value\"}");
function curl_get($url, array $options = array()) 
{ 
    $defaults = array( 
        CURLOPT_HEADER => 0, 
        CURLOPT_URL => $url, 
        CURLOPT_FRESH_CONNECT => 1, 
        CURLOPT_RETURNTRANSFER => 1, 
        CURLOPT_FORBID_REUSE => 1, 
        CURLOPT_TIMEOUT => 4, 
    ); 

    $ch = curl_init(); 
    curl_setopt_array($ch, ($options + $defaults)); 
    $result = curl_exec($ch);
	echo $result . "\n";
    $result = curl_exec($ch);
	echo $result . "\n";
    $result = curl_exec($ch);
	echo $result . "\n";
    $result = curl_exec($ch);
	echo $result . "\n";
    $result = curl_exec($ch);
	echo $result . "\n";
    curl_close($ch); 
    return $result; 
} 

?>
