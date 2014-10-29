#!/bin/bash

increment (){
   local sum
   let "sum = $1 + 1"
   return $sum
}

increment 5

echo "The result is $?"
