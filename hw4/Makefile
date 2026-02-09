build: hw4.out collatz.out

hw4.out: hw4.asm rars1_6.jar
	java -jar ./rars1_6.jar ./hw4.asm > hw4.out

collatz.out: collatz.py
	python3 collatz.py > collatz.out

check: collatz.out hw4.out
	tail -n +3 hw4.out | diff collatz.out -

clean:
	rm -f hw4.out collatz.out rars1_6.jar
