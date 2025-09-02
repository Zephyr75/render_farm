cold start time
hot start time
gpu support?
arm support?
snap start?
couts d'ingress hyper chers lambda
deploiement sur openshift
connection db extérieure


# Use case 1 : API

- Easy to deploy
- Docker integration
- Cold start time
- Connection to other OpenFaaS service
- Existing template
- Cost

# Use case 2 : Render farm

- Custom Dockerfile
- Cronjob
- Monitoring performance
- Scalable
- Error handling
- GPU ?
- Configuration of resources
- Cost


plateformes self service wavestone : joseph deriaux
nodepool gpu
qu est ce qui spawn
hpa?
kubetl get all voir ce qui spawn

faire schema spawn pour rendu
schema darchi

---


openshift = predefined kubernetes easier but more rigid

faas-netes is the bridge between openfaas and kubernetes, it translates openfaas function deployments into kubernetes resources (services, deployments, ...), communicates with kube's Horizontal Pod Autoscaler to manage function replicas

NATS streaming handles requests while functions are being loaded to allow returning an "accepted" response instantly and processing when available

![](of-workflow.png)


---

fonctionnalite
cout

config par rapport a lambda
comp cout aks

securite
bonnes pratiques integrees?

scanner vuln, patchs vitesse

grip trivi
est ce quils sont proactifs sur les fix

quest ce quoin peut automatiser
comment on peut mettre a dispo d'autres equipes

cas d'usage gros workload

facile de faire les deux docker const et faas

estimation cout d'usages


support
contexte
cas dusge
resultate
proj diff cout fonction secu
implementation ideale recommandee

