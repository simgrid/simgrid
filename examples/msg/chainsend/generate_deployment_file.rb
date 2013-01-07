#!/usr/bin/env ruby

require 'rexml/document'

class HostsExtractor
  @@doc = nil
  @@hosts = []

  def initialize(xml)
    @@doc = REXML::Document.new(xml)
    @@doc.elements.each('platform') do |platform|
      extract_hosts(platform)
    end
  end

  def extract_hosts(doc)
    doc.elements.each('AS') do |as|
      extract_hosts_from_AS(as)
      extract_hosts(as)
    end
  end

  def extract_hosts_from_AS(doc)
    doc.elements.each('host') do |h|
      @@hosts << h.attributes['id']
      puts "hosts %s" % h.attributes['id']
    end

    doc.elements.each('cluster') do |c|
      prefix = c.attributes['prefix']
      suffix = c.attributes['suffix']
      puts "%s %s %s" % [prefix, c.attributes['radical'], suffix]
      expand_radical(c.attributes['radical']).each do |num|
        @@hosts << "%s%s%s" % [prefix, num, suffix]
      end
    end
  end

  def hosts
    return @@hosts
  end

  def expand_radical(radical)
   l = []
   puts radical
   radical.split(',').each do |range|
     range.scan(/^\d+$/) { |x| l << x }
     range.scan(/^(\d+)-(\d+)$/) { |x, y| (x..y).each do |i| l << i end }
   end
   return l
  end
end

class DeploymentGenerator
  @@outfile = nil

  def initialize(fname)
    @@outfile = File.new(fname, "w")
  end

  def write_header
    @@outfile.puts "<?xml version='1.0'?>"
    @@outfile.puts "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">"
    @@outfile.puts "<platform version=\"3\">"
  end

  def write_process(name, function, hosts, args)
    @@outfile.puts "  <!-- %s -->" % name
    hosts.zip(args).each do |h, a|
      @@outfile.puts "  <process host=\"%s\" function=\"%s\">" % [h, function]
      @@outfile.puts "    <argument value=\"%s\"/>" % [a]
      @@outfile.puts "  </process>"
    end
  end

  def write_footer
    @@outfile.puts "</platform>"
  end
end

xml = File.read(ARGV.shift)
he = HostsExtractor.new(xml)

raise "Cannot run with less than 2 hosts" unless he.hosts.size > 1

output = ARGV.shift
n = ARGV.shift
if n == nil or n.to_i < 2
  n = he.hosts.size - 1
else
  n = n.to_i - 1
end
puts n

dg = DeploymentGenerator.new(output)
dg.write_header

puts he.hosts
broadcaster = he.hosts.shift
peers = he.hosts

dg.write_process("Broadcaster", "broadcaster", [broadcaster], [n])
dg.write_process("Peers", "peer", peers[0..n-1], (1..n))

dg.write_footer
