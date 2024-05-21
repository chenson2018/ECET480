factors=(0 2 4 10 20 50 100)

for factor in ${factors[@]}; do
  echo "Factor: $factor"
  ./main loop_unroll.bc optimized.bc $factor > /dev/null 2>&1
  llvm-link optimized.bc util/print.bc -o linked.bc
  llc -filetype=obj linked.bc -o linked.o
  clang linked.o -o out
  time ./out
  echo ""
  rm optimized.bc linked.bc linked.o out
done
