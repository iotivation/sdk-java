import org.iochibity.OCF;
import org.iochibity.CallbackParam;
import org.iochibity.DeviceAddress;
import org.iochibity.HeaderOption;
import org.iochibity.Payload;
import org.iochibity.PayloadResource;
import org.iochibity.Property;
import org.iochibity.RequestIn;
import org.iochibity.Resource;
// import org.iochibity.Resource$InstanceId;
import org.iochibity.ResourceManager;
import org.iochibity.ResourceServiceProvider;
import org.iochibity.Response;
import org.iochibity.ResponseOut;
import org.iochibity.constants.OCMethod;
import org.iochibity.constants.OCMode;
import org.iochibity.constants.OCStackResult;
import org.iochibity.constants.ResourcePolicy;
import org.iochibity.constants.ServiceResult;
import org.iochibity.exceptions.OCFNotImplementedException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.ArrayList;
import java.util.LinkedList;

public class OCFTest
{
    static{
	try{
	    System.loadLibrary("ocfjni_c");
	}catch(Exception e){
	    System.out.println(e.toString());
	}
    }

    static private void logResourcePolicies(Resource resource)
    {
	System.out.println("RESOURCE: policies: "
			   + String.format("0x%X", resource.policies & 0xFFFFF));
	if ( (resource.policies & ResourcePolicy.DISCOVERABLE) > 0 )
	    {System.out.println("DISCOVERABLE");}
	if ( (resource.policies & ResourcePolicy.ACTIVE) > 0 ) {System.out.println("ACTIVE");}
	if ( (resource.policies & ResourcePolicy.OBSERVABLE) > 0) {System.out.println("OBSERVABLE");}
	if ( (resource.policies & ResourcePolicy.SECURE) > 0) {System.out.println("SECURE");}
    }

    static private void logResource(Resource resource)
    {
	System.out.println("RESOURCE: logResource ENTRY");
	System.out.println("RESOURCE: resource uri: " + resource.uri);

	LinkedList<String> tll = resource.getTypes();
	tll.forEach(t -> System.out.println("RESOURCE: type:     " + t)); // Java 8

	LinkedList<String> ifll = resource.getInterfaces();
	ifll.forEach(iface -> System.out.println("RESOURCE: interface: " + iface));
	// for (int i = 0; i < ifll.size(); i++) {   // Java 7
	//     System.out.println("REQUEST IN: resource if:    " + ifll.get(i));
	// }

	LinkedList<Property> pll = resource.getProperties();
	System.out.println("RESOURCE: properties count: " + pll.size());
	pll.forEach(p -> System.out.println("RESOURCE: property: " + p.name + " = " + p.value));

	LinkedList<Resource> cll = resource.getChildren();
	System.out.println("RESOURCE: child resources count: " + cll.size());
	cll.forEach(ch -> System.out.println("RESOURCE: child resource: " + ch));

	System.out.println("RESOURCE: service provider (callback): "
			   + resource.getServiceProvider().getClass().getName());

	System.out.println("RESOURCE: callback param: "
			   + resource.getCallbackParam().getClass().getName());

	System.out.println("RESOURCE: serial number: " + resource.sn);

	// Instance Id
	// System.out.println("RESOURCE: resource instance id: " + resource.id.getClass().getName());
	if (resource.id.getClass().getName().equals("org.iochibity.Resource$InstanceId")) {
	    System.out.println("RESOURCE: resource InstanceId class: InstanceId");
	} else if (resource.id.getClass().getName().equals("org.iochibity.Resource$InstanceOrdinal")) {
		System.out.println("RESOURCE: resource InstanceId class: InstanceOrdinal, val="
				   + ((org.iochibity.Resource.InstanceOrdinal)resource.id).val);
	} else if (resource.id.getClass().getName().equals("org.iochibity.Resource$InstanceString")) {
		System.out.println("RESOURCE: resource InstanceId class: InstanceString, val="
				   + ((org.iochibity.Resource.InstanceString)resource.id).val);
	} else if (resource.id.getClass().getName().equals("org.iochibity.Resource$InstanceUuid")) {
		System.out.println("RESOURCE: resource InstanceId class: InstanceUuid, val="
				   + ((org.iochibity.Resource.InstanceUuid)resource.id).val);
	}

	System.out.println("RESOURCE: policies: " + resource.policies);
	logResourcePolicies(resource);

	try {
	    System.out.println("RESOURCE: action set: " + resource.getActionSet());
	} catch (OCFNotImplementedException e) {
	    System.out.println("ERROR**** RESOURCE: getActionSet not implemented.");
	}
	System.out.println("RESOURCE: logResource EXIT");
    }

    static private void logPayload(PayloadResource p)
    {
	// System.out.println("PAYLOAD: logPayload ENTRY");
	System.out.println("PAYLOAD: uri: " + p.uri);
	// System.out.println("PAYLOAD: logPayload EXIT");
    }

    static private void logRequestIn(RequestIn request)
    {
	System.out.println("REQUEST IN: logRequestIn ENTRY");
	System.out.println("REQUEST IN: handle at origin: " + request.handleAtOrigin);
	System.out.println("REQUEST IN: request method: " + request.method);
	System.out.println("REQUEST IN: query : \"" + request.query + "\"");
	System.out.println("REQUEST IN: msg id : " + request.messageId);

	Resource resource = request.getResource();
	logResource(resource);

	System.out.println("REQUEST IN: devaddr adapter: " + request.deviceAddress.adapter);
	System.out.println("REQUEST IN: devaddr flags: " + String.format("0x%04X", request.deviceAddress.flags & 0xFFFFF));
	System.out.println("REQUEST IN: devaddr port: " + request.deviceAddress.port);
	System.out.println("REQUEST IN: devaddr address: " + request.deviceAddress.address);
	System.out.println("REQUEST IN: devaddr ifindex: " + request.deviceAddress.ifindex);
	System.out.println("REQUEST IN: devaddr route data: " + request.deviceAddress.routeData);

	System.out.println("REQUEST IN: observe action: " + request.observeAction);
	System.out.println("REQUEST IN: observe id    : " + request.observeId);

	ArrayList<HeaderOption> vendorOptions = request.getVendorHeaderOptions();
	System.out.println("REQUEST IN: vendor opts ct: " + vendorOptions.size());

	Payload payload = request.getPayload();
	if (payload != null) {
	    System.out.println("REQUEST IN: payload type: " + payload.type);
	} else {
	    System.out.println("REQUEST IN: payload is null");
	}
    }

    static public class MyParam extends CallbackParam
    {
	int foo;
	public MyParam(int i) { foo = i; }
    }

    static public class TemperatureServiceProvider extends ResourceServiceProvider
    {
	static private LinkedList<PayloadResource> serviceGetRequest(int flag, RequestIn request, Object callbackParam)
	{
	    System.out.println("TEST serviceGetRequest ENTRY");
	    Resource r = request.getResource();
	    // PayloadResource payload = new PayloadResource();
	    LinkedList<PayloadResource> payloads = r.getPayloads();
	    payloads.forEach(pl -> logPayload(pl));
	    // OCRepPayloadSetPropInt(payload, "temp", 72);
	    // pl.putInt("temp", 72); // since props is a hashmap
	    // or:  pl.getProperties() returns a hashmap ...

	    return payloads;
	}

	@Override
	public int service(int flag, RequestIn request, Object callbackParam)
	{
	    System.out.println("TEST TemperatureServiceProvider.service routine ENTRY");
	    logRequestIn(request);

	    LinkedList<PayloadResource> payloads;

	    switch (request.method) {
	    case OCMethod.GET:
		payloads = serviceGetRequest(flag, request, callbackParam);
		break;
	    case OCMethod.PUT:
		System.out.println("TEST method: PUT");
		break;
	    case OCMethod.POST:
		System.out.println("TEST method: POST");
		break;
	    case OCMethod.DELETE:
		System.out.println("TEST method: DELETE");
		break;
	    case OCMethod.OBSERVE:
		break;
	    case OCMethod.OBSERVE_ALL:
		break;
	    case OCMethod.CANCEL_OBSERVE:
		break;
	    case OCMethod.PRESENCE:
		break;
	    case OCMethod.DISCOVER:
		break;
	    case OCMethod.NOMETHOD:
		break;
	    default:
		break;
	    }

	    /* now populate ResponseOut object with payload, etc. */
	    // ResponseOut response = new ResponseOut(request, payloads);
	    ResponseOut response = new ResponseOut();

	    /* finally, send ResponeOut */
	    OCF.sendResponse(response);

	    /* 2. create a ResponseOut object */
	    /* 3. populate the ResponseOut */
	    /* 4. send the ResponseOut */


	    //     /* *payload = new_payload; */
	    //     // If the result isn't an error or forbidden, send response
	    //     if (!((ehResult == OC_EH_ERROR) || (ehResult == OC_EH_FORBIDDEN)))
	    // 	{
	    // 	    // Format the response.  Note this requires some info about the request
	    // 	    response.requestHandle = crequest_in->requestHandle;
	    // 	    response.resourceHandle = crequest_in->resource;
	    // 	    response.ehResult = ehResult;
	    // 	    response.payload = (OCPayload*)new_payload; /* testing */
	    // 	    // Indicate that response is NOT in a persistent buffer
	    // 	    response.persistentBufferFlag = 0;

	    // 	    // Send the response
	    // 	    if (OCDoResponse(&response) != OC_STACK_OK)
	    // 		{
	    // 		    printf("Error sending response\n");
	    // 		    ehResult = OC_EH_ERROR;
	    // 		}
	    // 	}
	    //     return ehResult;

	    System.out.println("ResourceServiceProvider.service EXIT");
	    return ServiceResult.OK;
	}
    }

    static Resource temperatureResource = new Resource();
    static TemperatureServiceProvider tempServiceProvider = new TemperatureServiceProvider();

    public static void main(String[] args)
    {
	Runtime.getRuntime().addShutdownHook(new Thread()
	    {
		@Override
		public void run()
		{
		    System.out.println("Shutdown hook running!");
		    OCF.stop();
		}
	    });

	// System.out.println(System.getProperty("java.library.path"));
	OCF.Init(null, 0, OCMode.SERVER, "/Users/gar/iotivity/kit/java/jni-c/data.d/oic_svr_db_server.dat");

	OCF.setPlatformInfo("Fartmaster",
				"Acme Novelties",
				"http://acme.example.org",
				"modelnbr", "mfgdate", "platversion",
				"osversion", "hwversion", "firmwareversion",
				"http://acme.example.org/support",
				new SimpleDateFormat("yyyy.MM.dd.HH.mm.ss").format(new Date()));

	OCF.setDeviceInfo("", //Fartmaster2020",
			  new String[] {"type1", "type2"},
			  "version-0.1",
			  new String[] {"dmversion-0.1"});

	System.out.println("resource count before: " + ResourceManager.resourceCount());

	int r = ResourceManager.registerResource(temperatureResource, // OCResourceHandle*
					     "core.temp",
					     "oc.mi.def",
					     "/a/temperature",
					     tempServiceProvider, // ResourceServiceProvider
					     new MyParam(99),     // callbackParam object
					     (byte)(ResourcePolicy.DISCOVERABLE
						    | ResourcePolicy.SECURE));
	// | OC_OBSERVABLE | OC_SECURE);

	if (r != OCStackResult.OK) {
	    System.out.println("TEMPERATURE resource service provider registration failed!");
	    // exit (EXIT_FAILURE);
	} else {
	    System.out.println("TEMPERATURE resource service provider registration succeeded!\n");
	}
	System.out.println("resource count after: " + ResourceManager.resourceCount());

	System.out.println("temperatureResource handle: " + temperatureResource.handle);
	System.out.println("temperatureResource uri: " + temperatureResource.uri);
	LinkedList<String> tll = temperatureResource.getTypes();
	System.out.println("temperatureResource type (first): " + tll.getFirst());
	LinkedList<String> ifll = temperatureResource.getInterfaces();
	System.out.println("temperatureResource if   (first): " + ifll.getFirst());

	System.out.println("temperatureResource policies: " + temperatureResource.policies);
	logResourcePolicies(temperatureResource);

        System.out.println("press q then ENTER to terminate");

	OCF.run();
	System.out.println("run returned: " + r);

        while(true){
	    try {
		Thread.sleep(2000);
		System.out.println("GUI thread loop");
		System.out.println("resource count: " + ResourceManager.resourceCount());
	    } catch (InterruptedException e) {
		e.printStackTrace();
		msgError(TAG, e.toString());
	    }

            // outputting data until the quit boolean flag is not set
            if(quit==true) break;
            // System.out.println("hello buddy");
        }

        // Thread t1=new Thread(new Quitter());
        // t1.start();
    }

    public static void msgError(final String tag ,final String text) {
        System.out.println("[E]" + tag + " | " + text);
    }

    private final static String TAG = OCFTest.class.getSimpleName();
    static BufferedReader in=new BufferedReader(new InputStreamReader(System.in));
    static boolean quit=false;
    static class Quitter implements Runnable {
	public void run(){
	    String msg = null;
	    // threading is waiting for the key Q to be pressed
	    while(true){
		try{
		    msg=in.readLine();
		}catch(IOException e){
		    e.printStackTrace();
		}

		if(msg.equals("Q")) {quit=true;break;}
	    }
	}
    }
}
