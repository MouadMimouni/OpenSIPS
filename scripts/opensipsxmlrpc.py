#!/usr/bin/python
from threading import Thread
import xmlrpclib
import sys
import getopt

NO_THREADS=1
URL = "http://127.0.0.1:8888"
QUIET = False
SUM = False

class Worker(Thread):
	def __init__(self,id, url, method, params):
		Thread.__init__(self)
		self.id = id
		self.url = url
		self.method = method
		self.params = params

	def run(self):
		server = xmlrpclib.ServerProxy(self.url)
		exec "out = server.%s(%s)" % (self.method, self.params)
		if not QUIET:
			print "Thread %d ended: %s" % (self.id, out)
		if SUM:
			print "Thread %d ended" % self.id

def usage():
	print "usage: %s [-n NO_THREADS] [-u|--url URL] [-q] [-s] method [params]" % sys.argv[0]
	sys.exit()

if __name__=="__main__":
	if len(sys.argv) == 1:
		usage()
	
	try:
		opts, args = getopt.getopt(sys.argv[1:], "n:u:qs", ["url"])
	except getopt.GetoptError, err:
		print str(err)
		sys.exit()
	
	index = 1
	for opt, value in opts:
		if opt == "-n":
			NO_THREADS = int(value)
			index += 2
		elif opt in ("-u", "--url"):
			URL = value
			index += 2
		elif opt == "-q":
			QUIET = True
			index += 1
		elif opt == "-s":
			SUM = True
			index += 1
		else:
			print "unknown option %s" % opt
			usage()
	
	if len(sys.argv[index:]) < 1:
		print "no method specified"

	params = ""
	if len(sys.argv[index+1:]) > 0:
		params = "'%s'" % sys.argv[index+1]
		for p in range(index+2, len(sys.argv[index+2:]) + index + 2):
			params = "%s,'%s'" % (params, sys.argv[p])

	threads = []
	for thread_id in range(NO_THREADS):
		thread = Worker(thread_id, URL, sys.argv[index], params)
		thread.start()
		threads.append(thread)

	for thread_id in range(NO_THREADS):
		threads[thread_id].join()
