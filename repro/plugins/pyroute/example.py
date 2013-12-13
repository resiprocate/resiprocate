
def on_load():
    '''Do initialisation when module loads'''
    print 'example: on_load invoked'

def provide_route(request_uri):
    '''Process a request URI and return the new request URI'''
    print 'example: request_uri = ' + request_uri
    routes = list()
    routes.append('sip:bob@example.org')
    routes.append('sip:alice@example.org')
    return routes

