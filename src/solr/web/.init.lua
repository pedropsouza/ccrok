local onServerStart = function()
  if assert(unix.fork()) == 0 then
    unix.execve('./solre.com', {'./solre.com', '-o', 'solr.json'});
    unix.exit(127);
  end
end

OnServerStart = onServerStart;

LaunchBrowser("");
