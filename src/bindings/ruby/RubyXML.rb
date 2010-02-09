require 'rubygems'
require 'nokogiri'
require 'open-uri'



class RubyXML 
  
    attr_accessor :host, :function, :args, :file, :doc
    
    def initialize()
    
      @host = Array.new()
      @function = Array.new()
      @args = Array.new()
      
    end
  
#     Parse Application File
    
    def parseApplication(fileName) 
      @file = File.new(fileName);
      @doc = Nokogiri::XML(@file)
      index = 0
      for process in @doc.root.xpath("//process")
	
	@host[index] = process['host']
	@function[index] = process['function']
	@args[index] = Array.new()
	arg_index = 0
	for arg in process.xpath("./argument")
	  
	  @args[index][arg_index] = arg['value']
	  arg_index += 1
	 end
	index += 1 
      end
      
      
      
       @file.close();
    end

#     Print All
    def printAll()
      
#       puts @host.size
        for i in 0..@host.size-1
	  puts "> Host :" + @host[i]
	  puts ">> Function :" + @function[i]
	  for j in 0..@args[i].size-1
	    puts ">>> Arguments :" + @args[i][j]
	 end
	end
    end
end


