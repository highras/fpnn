<?php
curl_post("http://localhost:9876/service/httpDemo?a=b", "{\"key\":5,\"key2\":\"value\"}");
function curl_post($url, $post = "{}", array $options = array()) 
{ 
    $defaults = array( 
        CURLOPT_POST => 1, 
        CURLOPT_HEADER => 0, 
        CURLOPT_URL => $url, 
        CURLOPT_FRESH_CONNECT => 1, 
        CURLOPT_RETURNTRANSFER => 1, 
        CURLOPT_FORBID_REUSE => 1, 
        CURLOPT_TIMEOUT => 4, 
        CURLOPT_POSTFIELDS => $post 
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
